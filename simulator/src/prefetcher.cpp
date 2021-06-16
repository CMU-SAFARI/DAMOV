/** $lic$
 * Copyright (C) 2012-2015 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "bithacks.h"
#include "event_recorder.h"
#include "prefetcher.h"
#include "timing_event.h"
#include "zsim.h"

#define DBG(args...) info(args)
//#define DBG(args...)

void StreamPrefetcher::setParents(uint32_t _childId, const g_vector < MemObject * >&parents, Network * network)
{
    childId = _childId;
    if (parents.size() != 1)
        panic("Must have one parent");
    if (network)
        panic("Network not handled");
    parent = parents[0];
}

void StreamPrefetcher::setChildren(const g_vector < BaseCache * >&children, Network * network)
{
    if (children.size() < 1)
        panic("Must have one children");
    if (network)
        panic("Network not handled");
    child = children[0];
}

void StreamPrefetcher::initStats(AggregateStat * parentStat)
{
    AggregateStat *s = new AggregateStat();
    s->init(name.c_str(), "Prefetcher stats");
    profAccesses.init("acc", "Accesses");
    s->append(&profAccesses);
    profPrefetches.init("pf", "Issued prefetches");
    s->append(&profPrefetches);
    profDoublePrefetches.init("dpf", "Issued double prefetches");
    s->append(&profDoublePrefetches);
    profPageHits.init("pghit", "Page/entry hit");
    s->append(&profPageHits);
    profHits.init("hit", "Prefetch buffer hits, short and full");
    s->append(&profHits);
    profShortHits.init("shortHit", "Prefetch buffer short hits");
    s->append(&profShortHits);
    profStrideSwitches.init("strideSwitches", "Predicted stride switches");
    s->append(&profStrideSwitches);
    profLowConfAccs.init("lcAccs", "Low-confidence accesses with no prefetches");
    s->append(&profLowConfAccs);
    parentStat->append(s);
}

uint64_t StreamPrefetcher::access(MemReq & req)
{
    uint64_t longerCycle, pfRespCycle, respCycle, reqCycle;
    uint32_t origChildId = req.childId;

    req.childId = childId;
    reqCycle = req.cycle;

    if (req.type != GETS) {
        respCycle = parent->access(req);
        req.childId = origChildId;
        return respCycle;       		//other reqs ignored, including stores
    }

    profAccesses.inc();

    EventRecorder *evRec = zinfo->eventRecorders[req.srcId];

    StreamPrefetcherEvent *newEv;
    TimingRecord nla, wbAcc, FirstFetchRecord, SecondFetchRecord;

    FirstFetchRecord.clear();
    SecondFetchRecord.clear();
    wbAcc.clear();

    longerCycle = pfRespCycle = respCycle = parent->access(req);

    if (likely(evRec && evRec->hasRecord()))
        wbAcc = evRec->popRecord();

    Address pageAddr = req.lineAddr >> 6;
    uint32_t pos = req.lineAddr & (64-1);
    uint32_t idx = 16;


    // This loop gets unrolled and there are no control dependences. Way faster than a break (but should watch for the avoidable loop-carried dep)
    for (uint32_t i = 0; i < 16; i++) {
        bool match = (pageAddr == tag[i]);
        idx = match?  i : idx;  // ccmov, no branch
    }

    if (idx == 16) {  // entry miss
        uint32_t cand = 16;
        uint64_t candScore = -1;
        //uint64_t candScore = 0;
        for (uint32_t i = 0; i < 16; i++) {
            if (array[i].lastCycle > reqCycle + 500) continue;  // warm prefetches, not even a candidate
            if (array[i].ts < candScore) {  // just LRU
                cand = i;
                candScore = array[i].ts;
            }
        }

        if (cand < 16) {
            idx = cand;
            array[idx].alloc(reqCycle);
            array[idx].lastPos = pos;
            array[idx].ts = timestamp++;
            tag[idx] = pageAddr;
        }
    } else {  // entry hit
        profPageHits.inc();
        Entry& e = array[idx];
        array[idx].ts = timestamp++;

        // 1. Did we prefetch-hit?
        bool shortPrefetch = false;
        if (e.valid[pos]) {
            uint64_t pfRespCycle = e.times[pos].respCycle;
            shortPrefetch = pfRespCycle > respCycle;
            e.valid[pos] = false;  // close, will help with long-lived transactions
            respCycle = MAX(pfRespCycle, respCycle);
            e.lastCycle = MAX(respCycle, e.lastCycle);
            profHits.inc();
            if (shortPrefetch) profShortHits.inc();
        }

        // 2. Update predictors, issue prefetches
        int32_t stride = pos - e.lastPos;
        if (e.stride == stride) {
            e.conf.inc();
            if (e.conf.pred()) {  // do prefetches
                int32_t fetchDepth = (e.lastPrefetchPos - e.lastPos)/stride;
                uint32_t prefetchPos = e.lastPrefetchPos + stride;
                if (fetchDepth < 1) {
                    prefetchPos = pos + stride;
                    fetchDepth = 1;
                }

                if (prefetchPos < 64 && !e.valid[prefetchPos]) {
                    MESIState state = I;
                    MemReq pfReq =
                       { req.lineAddr + prefetchPos - pos, GETS, req.childId, &state, reqCycle, req.childLock,
                            state, req.srcId, MemReq::PREFETCH
                    };
                    pfRespCycle = parent->access(pfReq);
                    longerCycle = (wbAcc.reqCycle > pfRespCycle) ? wbAcc.reqCycle : pfRespCycle;

					          e.valid[prefetchPos] = true;
                    e.times[prefetchPos].fill(reqCycle, longerCycle);

                    profPrefetches.inc();

                    newEv = new(evRec) StreamPrefetcherEvent(0, longerCycle, evRec);
                    nla = { pfReq.lineAddr, longerCycle, longerCycle, pfReq.type, newEv, newEv};

                    if (wbAcc.isValid())
                         newEv->setAccessRecord(wbAcc, pfReq.cycle);

                    if (evRec && evRec->hasRecord()) {
                         FirstFetchRecord = evRec->popRecord();
                         newEv->setNextFetchRecord(FirstFetchRecord, pfReq.cycle);
                    }

                    if (shortPrefetch && fetchDepth < 8 && prefetchPos + stride < 64 && !e.valid[prefetchPos + stride]) {
                        prefetchPos += stride;
                        pfReq.lineAddr += stride;
                        pfRespCycle = parent->access(pfReq);
						            longerCycle = (likely(longerCycle < pfRespCycle)) ? pfRespCycle : longerCycle;

                        e.valid[prefetchPos] = true;
                        e.times[prefetchPos].fill(reqCycle, longerCycle);
                        profPrefetches.inc();
                        profDoublePrefetches.inc();
                        if (likely(nla.respCycle < longerCycle)) {
                             nla.respCycle = longerCycle;
                        }
                        if (likely(evRec && evRec->hasRecord())) {
                            info("[%s] PFL the transaction had generated a wb event!", name.c_str());
                            SecondFetchRecord = evRec->popRecord();
                            newEv->setStrideFetchRecord(SecondFetchRecord, pfReq.cycle);
                        }
                    }
                    e.lastPrefetchPos = prefetchPos;
                    assert(state == I);  // prefetch access should not give us any permissions

                    evRec->pushRecord(nla);
                    req.childId = origChildId;
                    return longerCycle;
                }
            } else {
                profLowConfAccs.inc();
            }
        } else {
            e.conf.dec();
            // See if we need to switch strides
            if (!e.conf.pred()) {
                int32_t lastStride = e.lastPos - e.lastLastPos;

                if (stride && stride != e.stride && stride == lastStride) {
                    e.conf.reset();
                    e.stride = stride;
                    profStrideSwitches.inc();
                }
            }
            e.lastPrefetchPos = pos;
        }

    e.lastLastPos = e.lastPos;
    e.lastPos = pos;
    }

    if (wbAcc.isValid())
            evRec->pushRecord(wbAcc);

    req.childId = origChildId;

    return longerCycle;
}

// nop for now; do we need to invalidate our own state?
uint64_t StreamPrefetcher::invalidate(const InvReq& req) {
    return child->invalidate(req);
}
