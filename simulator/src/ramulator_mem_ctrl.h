#ifndef RAMULATOR_MEM_CTRL_H_
#define RAMULATOR_MEM_CTRL_H_

#include <map>
#include <set>
#include <string>
#include <functional>
#include "g_std/g_string.h"
#include "memory_hierarchy.h"
#include "pad.h"
#include <list>
#include "stats.h"
#include "StatType.h"

using namespace std;

namespace ramulator {
  class Request;
  class RamulatorWrapper;
};

class RamulatorAccEvent;
class Ramulator : public MemObject { //one Ramulator controller
  private:
    static int gcd(int u, int v) {
      if (v > u) {
        swap(u,v);
      }

      while (v != 0) {
        int r = u % v;
        u = v;
        v = r;
      }

      return u;
    }


    g_string name;
    uint32_t domain;
    uint32_t minLatency;
    uint32_t cpuFreq;
    uint32_t clockDivider;
    double tCK;
    double memFreq;
    unsigned freqRatio;
    bool pim_mode;
    unsigned long long tickCounter = 0;
    int cpu_tick, mem_tick, tick_gcd;
    string application_name;
    ramulator::RamulatorWrapper* wrapper;

    std::multimap<uint64_t, RamulatorAccEvent*> inflightRequests;

    uint64_t curCycle; //processor cycle, used in callbacks

    // R/W stats
    PAD();
    Counter profReads;
    Counter profWrites;
    Counter profTotalRdLat;
    Counter profTotalWrLat;
  	Counter reissuedAccesses;
    PAD();
    int inflight_r = 0;
    int inflight_w = 0;

  public:
    Ramulator(std::string config_file, unsigned num_cpus, unsigned cache_line_size, uint32_t _minLatency, uint32_t _domain, const g_string& _name, bool pim_mode,  const string& application, unsigned _cpuFreq, bool _record_memory_trace, bool networkOverhead);
    ~Ramulator();
    void finish();

    const char* getName() {return name.c_str();}
    void initStats(AggregateStat* parentStat);

    // Record accesses
    uint64_t access(MemReq& req);

    // Event-driven simulation (phase 2)
    uint32_t tick(uint64_t cycle);
    void enqueue(RamulatorAccEvent* ev, uint64_t cycle);

  private:
    std::function<void(ramulator::Request&)> read_cb_func;
	  std::function<void(ramulator::Request&)> write_cb_func;
	  bool resp_stall;
	  bool req_stall;

    void DRAM_read_return_cb(ramulator::Request&);
    void DRAM_write_return_cb(ramulator::Request&);
	  unsigned m_num_cores;

    vector<RamulatorAccEvent> ramulatorAccEvent;
    set<uint64_t> inflightCheck;
    set<uint64_t> inflightCheck2;
    map<uint64_t, uint64_t> addr_counter;

    std::list<RamulatorAccEvent*> overflowQueue;
};

#endif  // RAMULATOR_MEM_CTRL_H_
