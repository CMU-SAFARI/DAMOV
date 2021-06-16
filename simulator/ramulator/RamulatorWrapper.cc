#include <map>

#include "RamulatorWrapper.h"
#include "Config.h"
#include "Request.h"
#include "MemoryFactory.h"
#include "Memory.h"
#include "DDR3.h"
#include "DDR4.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "GDDR5.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"

using namespace ramulator;

static map<string, function<MemoryBase *(const Config&, int)> > name_to_func = {
    {"DDR3", &MemoryFactory<DDR3>::create}, {"DDR4", &MemoryFactory<DDR4>::create},
    {"LPDDR3", &MemoryFactory<LPDDR3>::create}, {"LPDDR4", &MemoryFactory<LPDDR4>::create},
    {"GDDR5", &MemoryFactory<GDDR5>::create},
    {"WideIO", &MemoryFactory<WideIO>::create}, {"WideIO2", &MemoryFactory<WideIO2>::create},
    {"HBM", &MemoryFactory<HBM>::create},
    {"SALP-1", &MemoryFactory<SALP>::create}, {"SALP-2", &MemoryFactory<SALP>::create},
    {"SALP-MASA", &MemoryFactory<SALP>::create},{"HMC", &MemoryFactory<HMC>::create},
};

RamulatorWrapper::RamulatorWrapper(const char* config_path, unsigned num_cpus, int cacheline, bool pim_mode, bool record_memory_trace, const char* application_name, bool networkOverhead)
{

    Config configs(config_path);
    configs.set_core_num(num_cpus);
    configs.set_pim_mode(pim_mode);
    string app_name(application_name);
    configs.set_network_overhead(networkOverhead);

    configs.set_application_name(app_name);
    configs.set_record_memory_trace(record_memory_trace);

    const string& std_name = configs["standard"];
    assert(name_to_func.find(std_name) != name_to_func.end() && "unrecognized standard name");
    mem = name_to_func[std_name](configs, cacheline);
    tCK = mem->clk_ns();
    std::cout << "[RAMULATOR] Initialized Ramulator" << std::endl;
}


RamulatorWrapper::~RamulatorWrapper() {
    delete mem;
}

void RamulatorWrapper::tick() {
    mem->tick();
}

bool RamulatorWrapper::send(Request req) {
    return mem->send(req);
}

void RamulatorWrapper::finish() {
  std::cout << "[RAMULATOR] Finished Ramulator" << std::endl;
  mem->finish();
}

double RamulatorWrapper::get_tCK() {
    return tCK;
}
