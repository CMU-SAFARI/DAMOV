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

#ifndef MESH_NETWORK_MD1_H_
#define MESH_NETWORK_MD1_H_

/* Models contention in a basic mesh network
 */

#include <string>
#include <unordered_map>
#include <vector>
#include "network.h"
#include "locks.h"
#include "mesh_router_md1.h"

struct RouteType {
    bool isDynamic;
    uint32_t staticDelay;
    uint32_t srcX;
    uint32_t srcY;
    uint32_t destX;
    uint32_t destY;
};

#define MESH_PORT_HOME 0
#define MESH_PORT_NORTH 1
#define MESH_PORT_EAST 2
#define MESH_PORT_SOUTH 3
#define MESH_PORT_WEST 4
#define MESH_PORT_RADIX 5

class MeshNetworkMD1 : public Network {
  private:
    std::unordered_map<std::string, RouteType> delayMap;
    std::vector<std::vector<MeshRouterMD1> > routers;
    uint32_t xDim, yDim;
    uint32_t hopDelay;
    lock_t lock;

    uint64_t lastUpdateCycle;
    uint64_t lastPhase;
    
    double step(double curCycle, uint32_t destX, uint32_t destY, uint32_t destPort);
    
  public:
    MeshNetworkMD1(const char* filename);
    virtual uint32_t getRTT(uint64_t curCycle, uint32_t latency, const char* src, const char* dst);
    virtual void initStats(AggregateStat*);
};

#endif  // MESH_NETWORK_H_

