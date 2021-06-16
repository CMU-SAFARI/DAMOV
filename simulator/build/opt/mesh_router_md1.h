#ifndef MESH_ROUTER_MD1_H_
#define MESH_ROUTER_MD1_H_

#include <unordered_map>
#include <vector>
#include "log.h"
#include "stats.h"

#define ROUTER_MD1_MAX_CHANNELS 8

typedef struct {
    double smoothedPhaseAccess = 0.0;
    uint64_t curPhaseAccess = 0;
    uint64_t totalAccess = 0;
    uint64_t totalPhases = 0;
    uint64_t totalCycles = 0;
    double curLatency = 0.0;
    uint64_t numClamped = 0;
    double maxLoad = 0.0;
} RouterMD1Channel;

class MeshRouterMD1 {

    uint32_t radix;

  public:
    RouterMD1Channel channels[ROUTER_MD1_MAX_CHANNELS];
    uint32_t coreId, xPos, yPos;
    MeshRouterMD1(uint32_t cId, uint32_t xPos, uint32_t yPos, uint32_t numPorts);
    void initStats(AggregateStat*);
    void updateLatency(uint64_t phaseCycles);
    double access(uint32_t chan);
};

#endif
