
#include <iostream>
#include <string>
#include <cstring>
#include "zsim.h"
#include "mesh_router_md1.h"

#define MESH_ROUTER_MD1_SMOOTHING 0.5
#define MESH_ROUTER_MD1_LOAD_FACTOR 2.5

// This model is only reasonably accurate for workloads with low network activity
// Warn user to switch to a more-accurate model when above this access frequency
#define MESH_ROUTER_MD1_CLAMP 0.5

MeshRouterMD1::MeshRouterMD1(uint32_t id, uint32_t xPos, uint32_t yPos, uint32_t numPorts) {
    this->coreId = id;
    this->xPos = xPos;
    this->yPos = yPos;
    radix = numPorts;
}

void MeshRouterMD1::initStats(AggregateStat* parentStat) {
    AggregateStat* routerStat = new AggregateStat();
    std::string routerDim = "("+std::to_string(xPos)+", "+std::to_string(yPos)+")";
    std::string routerCore = std::to_string(coreId);

    std::string routerStatName = "router_"+routerCore;
    std::string routerStatDesc = "Router at position "+routerDim+" stats";
    char* cRouterStatName = new char[routerStatName.length()+1];
    char* cRouterStatDesc = new char[routerStatDesc.length()+1];
    std::strcpy(cRouterStatName, routerStatName.c_str());
    std::strcpy(cRouterStatDesc, routerStatDesc.c_str());
    routerStat->init(cRouterStatName, cRouterStatDesc);

    auto z = [this](){
        double maxLoad = 0;
        for(uint32_t i = 0; i < radix; i++) {
            if(channels[i].maxLoad > maxLoad)
                maxLoad = channels[i].maxLoad;
        }
        return maxLoad;
    };
    LambdaStat<decltype(z)>* loadStat = new LambdaStat<decltype(z)>(z);
    loadStat->init("maxLoad", "Maximum router load per phase");
    routerStat->append(loadStat);

    for(uint32_t i = 0; i < radix; i++) {
        std::string accessStatName = "c"+std::to_string(i)+"_accesses";
        std::string accessStatDesc = "Channel "+std::to_string(i)+" total accesses";
        char* cAccessStatName = new char[accessStatName.length()+1];
        char* cAccessStatDesc = new char[accessStatDesc.length()+1];
        std::strcpy(cAccessStatName, accessStatName.c_str());
        std::strcpy(cAccessStatDesc, accessStatDesc.c_str());

        std::string clampStatName = "c"+std::to_string(i)+"_clamps";
        std::string clampStatDesc = "Channel "+std::to_string(i)+" total clamps";
        char* cClampStatName = new char[clampStatName.length()+1];
        char* cClampStatDesc = new char[clampStatDesc.length()+1];
        std::strcpy(cClampStatName, clampStatName.c_str());
        std::strcpy(cClampStatDesc, clampStatDesc.c_str());

        ProxyStat* accessStat = new ProxyStat();
        accessStat->init(cAccessStatName, cAccessStatDesc, &(channels[i].totalAccess));
        ProxyStat* clampedStat = new ProxyStat();
        clampedStat->init(cClampStatName, cClampStatDesc, &(channels[i].numClamped));

        routerStat->append(accessStat);
        routerStat->append(clampedStat);
    }

    parentStat->append(routerStat);
}

void MeshRouterMD1::updateLatency(uint64_t phaseCycles) {

    for(uint64_t i = 0; i < radix; i++) {
        channels[i].totalAccess += channels[i].curPhaseAccess;

        // Calculate exponential moving average
        channels[i].smoothedPhaseAccess = (channels[i].smoothedPhaseAccess * MESH_ROUTER_MD1_SMOOTHING) + 
                                          (channels[i].curPhaseAccess * (1 - MESH_ROUTER_MD1_SMOOTHING));

        // Requests per cycle
        double load = channels[i].smoothedPhaseAccess/((double)phaseCycles);

        // Hack to attempt to model non-uniform network access
        // by pessimistically increasing network access frequency
        double adjLoad = load * MESH_ROUTER_MD1_LOAD_FACTOR;

        if(load > channels[i].maxLoad)
            channels[i].maxLoad = load;

        channels[i].totalCycles += phaseCycles;

        if(adjLoad > MESH_ROUTER_MD1_CLAMP) {
            adjLoad = MESH_ROUTER_MD1_CLAMP;
            channels[i].numClamped++;
        }

        double avgWait = adjLoad/(2.0*(1.0 - adjLoad));
        channels[i].curLatency = avgWait;
        channels[i].curPhaseAccess = 0;
    }
}


double MeshRouterMD1::access(uint32_t chan) {
    assert(chan < radix);

    __sync_fetch_and_add(&channels[chan].curPhaseAccess, 1);

    return channels[chan].curLatency;
}

