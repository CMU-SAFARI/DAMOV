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

#include "accelerator_core.h"
#include "filter_cache.h"
#include "zsim.h"

#define DEBUG_MSG(args...)
//#define DEBUG_MSG(args...) info(args)

AcceleratorCore::AcceleratorCore(FilterCache* _l1i, FilterCache* _l1d, uint32_t _domain, g_string& _name)
    : Core(_name), l1i(_l1i), l1d(_l1d), instrs(0), curCycle(0), cRec(_domain, _name) {}

uint64_t AcceleratorCore::getPhaseCycles() const {
    return curCycle % zinfo->phaseLength;
}

void AcceleratorCore::initStats(AggregateStat* parentStat) {
    AggregateStat* coreStat = new AggregateStat();
    coreStat->init(name.c_str(), "Core stats");

    auto x = [this]() { return cRec.getUnhaltedCycles(curCycle); };
    LambdaStat<decltype(x)>* cyclesStat = new LambdaStat<decltype(x)>(x);
    cyclesStat->init("cycles", "Simulated unhalted cycles");
    coreStat->append(cyclesStat);

    auto y = [this]() { return cRec.getContentionCycles(); };
    LambdaStat<decltype(y)>* cCyclesStat = new LambdaStat<decltype(y)>(y);
    cCyclesStat->init("cCycles", "Cycles due to contention stalls");
    coreStat->append(cCyclesStat);

    ProxyStat* instrsStat = new ProxyStat();
    instrsStat->init("instrs", "Simulated instructions", &instrs);
    coreStat->append(instrsStat);

    parentStat->append(coreStat);
}


void AcceleratorCore::contextSwitch(int32_t gid) {
    if (gid == -1) {
        l1i->contextSwitch();
        l1d->contextSwitch();
    }
}

void AcceleratorCore::join() {
    DEBUG_MSG("[%s] Joining, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
    curCycle = cRec.notifyJoin(curCycle);
    phaseEndCycle = zinfo->globPhaseCycles + zinfo->phaseLength;
    DEBUG_MSG("[%s] Joined, curCycle %ld phaseEnd %ld", name.c_str(), curCycle, phaseEndCycle);
}

void AcceleratorCore::leave() {
    cRec.notifyLeave(curCycle);
}

void AcceleratorCore::loadAndRecord(Address addr, uint32_t size) {
    uint64_t startCycle = curCycle;
    curCycle = l1d->load(addr, curCycle);
    cRec.record(startCycle);
}

void AcceleratorCore::finish(){
    return;
}
void AcceleratorCore::storeAndRecord(Address addr, uint32_t size) {
    uint64_t startCycle = curCycle;
    curCycle = l1d->store(addr, curCycle);
    cRec.record(startCycle);
}

void AcceleratorCore::bblAndRecord(Address bblAddr, BblInfo* bblInfo) {
    assert(bblInfo->depth > 0);

    instrs += bblInfo->instrs;
    curCycle += bblInfo->depth;

    if(offload_region){
        offload_instrs += bblInfo->instrs;
    }


    Address endBblAddr = bblAddr + bblInfo->bytes;
    for (Address fetchAddr = bblAddr; fetchAddr < endBblAddr; fetchAddr+=(1 << lineBits)) {
        uint64_t startCycle = curCycle;
        curCycle = l1i->load(fetchAddr, curCycle);
        cRec.record(startCycle);
    }
}

InstrFuncPtrs AcceleratorCore::GetFuncPtrs() {
    return {LoadAndRecordFunc, StoreAndRecordFunc, BblAndRecordFunc, BranchFunc, PredLoadAndRecordFunc, PredStoreAndRecordFunc, OffloadBegin, OffloadEnd, FPTR_ANALYSIS, {0}};
}

void AcceleratorCore::OffloadBegin(THREADID tid) {
    static_cast<AcceleratorCore*>(cores[tid])->offloadFunction_begin();
}
void AcceleratorCore::OffloadEnd(THREADID tid) {
    static_cast<AcceleratorCore*>(cores[tid])->offloadFunction_end();
}
void AcceleratorCore::LoadAndRecordFunc(THREADID tid, ADDRINT addr, UINT32 size) {
    static_cast<AcceleratorCore*>(cores[tid])->loadAndRecord(addr, size);
}

void AcceleratorCore::StoreAndRecordFunc(THREADID tid, ADDRINT addr, UINT32 size) {
    static_cast<AcceleratorCore*>(cores[tid])->storeAndRecord(addr, size);
}

void AcceleratorCore::BblAndRecordFunc(THREADID tid, ADDRINT bblAddr, BblInfo* bblInfo) {
    AcceleratorCore* core = static_cast<AcceleratorCore*>(cores[tid]);
    core->bblAndRecord(bblAddr, bblInfo);

    while (core->curCycle > core->phaseEndCycle) {
        core->phaseEndCycle += zinfo->phaseLength;
        uint32_t cid = getCid(tid);
        uint32_t newCid = TakeBarrier(tid, cid);
        if (newCid != cid) break; /*context-switch*/
    }
}

void AcceleratorCore::PredLoadAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred, UINT32 size) {
    if (pred) static_cast<AcceleratorCore*>(cores[tid])->loadAndRecord(addr, size);
}

void AcceleratorCore::PredStoreAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred, UINT32 size) {
    if (pred) static_cast<AcceleratorCore*>(cores[tid])->storeAndRecord(addr, size);
}
