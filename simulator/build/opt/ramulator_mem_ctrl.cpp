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

#include "ramulator_mem_ctrl.h"
#include <map>
#include <string>
#include "event_recorder.h"
#include "tick_event.h"
#include "timing_event.h"
#include "zsim.h"
#include "RamulatorWrapper.h"
#include "Request.h"

using namespace std; // NOLINT(build/namespaces)

class RamulatorAccEvent : public TimingEvent {
  private:
    Ramulator* dram;
    bool write;
    Address addr;
    uint32_t coreid;
  public:
    uint64_t sCycle;
    RamulatorAccEvent(Ramulator* _dram, bool _write, Address _addr, int32_t domain, uint32_t _coreid) :
            TimingEvent(0, 0, domain), dram(_dram), write(_write), addr(_addr), coreid(_coreid) {}

    bool isWrite() const {
      return write;
    }

    Address getAddr() const {
      return addr;
    }

    uint32_t getCoreID() const{
      return coreid;
    }

    void simulate(uint64_t startCycle) {
      sCycle = startCycle;
      dram->enqueue(this, startCycle);
    }
};


Ramulator::Ramulator(std::string config_file, unsigned num_cpus, unsigned cache_line_size, uint32_t _minLatency, uint32_t _domain,
  const g_string& _name, bool pim_mode, const string& application,
  unsigned _cpuFreq, bool _record_memory_trace, bool _networkOverhead):
	wrapper(NULL),
	read_cb_func(std::bind(&Ramulator::DRAM_read_return_cb, this, std::placeholders::_1)),
	write_cb_func(std::bind(&Ramulator::DRAM_write_return_cb, this, std::placeholders::_1)),
	resp_stall(false),
	req_stall(false)
{
  minLatency = _minLatency;
  m_num_cores=num_cpus;
  const char* config_path = config_file.c_str();
  string pathStr = zinfo->outputDir;
  cout << pathStr << " " << application << endl;
  application_name = pathStr+"/"+application;
  const char* app_name = application_name.c_str();

  wrapper = new ramulator::RamulatorWrapper(config_path, num_cpus, cache_line_size, pim_mode, _record_memory_trace, app_name, _networkOverhead);

  cpu_tick = int(1000000.0/_cpuFreq);
  mem_tick = wrapper->get_tCK()*1000;
  if(pim_mode) cpu_tick = mem_tick;

  tick_gcd = gcd(cpu_tick, mem_tick);
  cpu_tick /= tick_gcd;
  mem_tick /= tick_gcd;

  tCK = wrapper->get_tCK();
  cpuFreq = _cpuFreq;
  clockDivider = 0;
  memFreq = (1/(tCK /1000000))/1000;
  info ("[RAMULATOR] Mem frequency %f", memFreq);
  this->pim_mode = pim_mode;
  if(pim_mode) cpuFreq = memFreq;
  freqRatio = ceil(cpuFreq/memFreq);
  info("[RAMILATOR] CPU/Mem frequency ratio %d", freqRatio);

  Stats_ramulator::statlist.output(pathStr+"/"+application+".ramulator.stats");
  curCycle = 0;
  domain = _domain;
  TickEvent<Ramulator>* tickEv = new TickEvent<Ramulator>(this, domain);
  tickEv->queue(0);  // start the sim at time 0
  name = _name;
}

Ramulator::~Ramulator(){
  delete wrapper;
}

void Ramulator::initStats(AggregateStat* parentStat) {
  AggregateStat* memStats = new AggregateStat();
  memStats->init(name.c_str(), "Memory controller stats");
  profReads.init("rd", "Read requests"); memStats->append(&profReads);
  profWrites.init("wr", "Write requests"); memStats->append(&profWrites);
  profTotalRdLat.init("rdlat", "Total latency experienced by read requests"); memStats->append(&profTotalRdLat);
  profTotalWrLat.init("wrlat", "Total latency experienced by write requests"); memStats->append(&profTotalWrLat);
  reissuedAccesses.init("reissuedAccesses", "Number of accesses that were reissued due to full queue"); memStats->append(&reissuedAccesses);
  parentStat->append(memStats);
}

uint64_t Ramulator::access(MemReq& req) {
  switch (req.type) {
    case PUTS:
    case PUTX:
      *req.state = I;
      break;
    case GETS:
      *req.state = req.is(MemReq::NOEXCL)? S : E;
      break;
    case GETX:
      *req.state = M;
      break;
    default: panic("!?");
  }

  if(req.type == PUTS){
    return req.cycle; //must return an absolute value, 0 latency
  }
  else {
    bool isWrite = (req.type == PUTX);
    uint64_t respCycle = req.cycle + minLatency;

    if (zinfo->eventRecorders[req.srcId]) {
      Address addr = req.lineAddr <<lineBits;
      RamulatorAccEvent* memEv = new (zinfo->eventRecorders[req.srcId]) RamulatorAccEvent(this, isWrite, addr, domain,req.srcId);
      memEv->setMinStartCycle(req.cycle);
      TimingRecord tr = {addr, req.cycle, respCycle, req.type, memEv, memEv};
      zinfo->eventRecorders[req.srcId]->pushRecord(tr);
    }
    return respCycle;
  }
}

uint32_t Ramulator::tick(uint64_t cycle) {
  // REMOVE comments for clock divider (i.e., memory clock is different from host clock)
  //if((tickCounter % freqRatio) == 0){
  wrapper->tick();
  //}
  //tickCounter++;

  if(overflowQueue.size() > 0){
    RamulatorAccEvent *ev = overflowQueue.front();
    if(ev->isWrite()){
      ramulator::Request req((long)ev->getAddr(), ramulator::Request::Type::WRITE, write_cb_func,ev->getCoreID());
      long addr_tmp = req._addr;

      if(wrapper->send(req)){
        overflowQueue.pop_front();
        inflight_w++;

        inflightRequests.insert(std::pair<uint64_t, RamulatorAccEvent*>((long)ev->getAddr(), ev));
        ev->hold();
      }
    }
    else {
      ramulator::Request req((long)ev->getAddr(), ramulator::Request::Type::READ, read_cb_func ,ev->getCoreID());
      long addr_tmp = req._addr;

      if(wrapper->send(req)){
        overflowQueue.pop_front();
        inflight_r++;

        inflightRequests.insert(std::pair<uint64_t, RamulatorAccEvent*>((long)ev->getAddr(), ev));
        ev->hold();
      }
    }
  }

  curCycle++;
  return 1;
}

void Ramulator::finish(){
  wrapper->finish();
  Stats_ramulator::statlist.printall();
}

void Ramulator::enqueue(RamulatorAccEvent* ev, uint64_t cycle) {
  long addr_tmp;

  if(ev->isWrite()){
    ramulator::Request req((long)ev->getAddr(), ramulator::Request::Type::WRITE, write_cb_func,ev->getCoreID());
    addr_tmp = req._addr;

    if(!wrapper->send(req)){
      overflowQueue.push_back(ev);
      reissuedAccesses.inc();
      return;
    }
      inflight_w++;
  }
  else {
    ramulator::Request req((long)ev->getAddr(), ramulator::Request::Type::READ, read_cb_func, ev->getCoreID());
    long addr_tmp = req._addr;

    if(!wrapper->send(req)){
      overflowQueue.push_back(ev);
      reissuedAccesses.inc();
      return;
    }

    inflight_r++;
  }

  inflightRequests.insert(std::pair<uint64_t, RamulatorAccEvent*>((long)ev->getAddr(), ev));
  ev->hold();
}

void Ramulator::DRAM_read_return_cb(ramulator::Request& req) {
  std::multimap<uint64_t, RamulatorAccEvent*>::iterator it = inflightRequests.find(req._addr);
  if(it == inflightRequests.end()){
    info("[RAMULATOR] I didn't request address %ld (%ld)", req._addr, req.addr);
  }

  assert((it != inflightRequests.end()));
  RamulatorAccEvent* ev = it->second;

  uint32_t lat = curCycle+1 - ev->sCycle;

  if (ev->isWrite()) {
    profWrites.inc();
    profTotalWrLat.inc(lat);
    inflight_w--;
  }
  else {
    profReads.inc();
    profTotalRdLat.inc(lat);
    inflight_r--;
  }

  ev->release();
  ev->done(curCycle+1);

  inflightRequests.erase(it);
}

void Ramulator::DRAM_write_return_cb(ramulator::Request& req) {
  //Same as read for now
  DRAM_read_return_cb(req);
}
