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

#include <fstream>
#include <string>
#include <iostream>
#include "log.h"
#include "mesh_network_md1.h"
#include "bithacks.h"
#include "zsim.h"

using std::ifstream;
using std::string;

// Network traffic phase length, a multiple of the ZSim simulator phase length
#define MESH_NETWORK_MD1_UPDATE_PHASES 100

MeshNetworkMD1::MeshNetworkMD1(const char* filename) {
    ifstream inFile(filename);

    if (!inFile) {
        panic("Could not open network description file %s", filename);
    }

    /* Header:
       x_dim y_dim hop_lat

       Two types of lines:

       src dest 0 lat
       src dest 1 src_x src_y dest_x dest_y
    */

    futex_init(&lock);

    inFile >> xDim;
    inFile >> yDim;
    inFile >> hopDelay;

    while (inFile.good()) {
        string src, dest;
        uint32_t delay = -1;
	       uint32_t type;
	        uint32_t srcX = -1, srcY = -1, destX = -1, destY = -1;
        inFile >> src;
        inFile >> dest;
	       inFile >> type;

        if (inFile.eof()) break;

	      if(type == 1) {
            inFile >> srcX;
            inFile >> srcY;
            inFile >> destX;
            inFile >> destY;
	      }
      	else if(type == 0) {
                  inFile >> delay;
      	}
      	else {
                  panic("Parsed unknown type! src: %s dest %s type %d", src.c_str(), dest.c_str(), type);
      	}

        string s1 = src + " " + dest;
        string s2 = dest + " " + src;

        assert((delayMap.find(s1) == delayMap.end()));
        assert((delayMap.find(s2) == delayMap.end()));

        delayMap[s1] = {type==1, delay, srcX, srcY, destX, destY};
        delayMap[s2] = {type==1, delay, destX, destY, srcX, srcY};
    }

    inFile.close();

    for(uint32_t x = 0; x < xDim; x++) {
        std::vector<MeshRouterMD1> tmpRV;
        routers.push_back(tmpRV);
        for(uint32_t y = 0; y < yDim; y++) {
            MeshRouterMD1 tmpR = MeshRouterMD1(y*yDim+x, x, y, MESH_PORT_RADIX);
            routers[x].push_back(tmpR);
        }
    }

    lastUpdateCycle = 0;
    lastPhase = 0;
}

void MeshNetworkMD1::initStats(AggregateStat* parentStat) {
    AggregateStat* netStat = new AggregateStat();
    netStat->init("network", "Mesh network stats");
    for(uint32_t y = 0; y < yDim; y++) {
        for(uint32_t x = 0; x < xDim; x++) {
            routers[x][y].initStats(netStat);
        }
    }

    parentStat->append(netStat);
}

double MeshNetworkMD1::step(double curCycle, uint32_t destX, uint32_t destY, uint32_t destPort) {
    double newCycle = curCycle;

    newCycle += hopDelay;

    newCycle += routers[destX][destY].access(destPort);

    return newCycle;
}


uint32_t MeshNetworkMD1::getRTT(uint64_t curCycle, uint32_t latency, const char* src, const char* dest) {
    string key(src);
    key += " ";
    key += dest;

    // If new phase, update all routers
    if(zinfo->numPhases > lastPhase) {
        futex_lock(&lock);
        // Recheck, someone may have updated already
        if(zinfo->numPhases > lastPhase) {
            uint64_t phaseCycle = (zinfo->numPhases - 1) * zinfo->phaseLength;
            if(lastUpdateCycle + (MESH_NETWORK_MD1_UPDATE_PHASES * zinfo->phaseLength) <= phaseCycle) {
                for(uint64_t x = 0; x < xDim; x++) {
                    for(uint64_t y = 0; y < yDim; y++) {
                        routers[x][y].updateLatency(phaseCycle - lastUpdateCycle);
                    }
                }

                lastUpdateCycle = phaseCycle;
            }
        }
        lastPhase = zinfo->numPhases;
        futex_unlock(&lock);
    }

    // Translate text src/dest to coordinates
    if(delayMap.find(key) == delayMap.end()) {
        panic("ERROR: mapping %s to %s not found!", src, dest);
    }
    RouteType route = delayMap[key];
    uint64_t curX = route.srcX;
    uint64_t curY = route.srcY;

    if(!route.isDynamic) {
        return 2*route.staticDelay;
    }

    // Calculate CPU -> router latency
    double stepCycle = step((double)0, curX, curY, MESH_PORT_HOME);
    // Route X
    while(curX != route.destX) {

        // Traveling W -> E
        if(curX < route.destX) {
            curX++;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_WEST);
        }
        // Traveling E -> W
        else {
            curX--;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_EAST);
        }
    }

    // Route Y
    while(curY != route.destY) {

        // Traveling N -> S
        if(curY < route.destY) {
            curY++;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_NORTH);
        }
        // Traveling S -> N
        else {
            curY--;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_SOUTH);
        }
    }

    // CPU -> router response latency
    stepCycle = step(stepCycle, curX, curY, MESH_PORT_HOME);

    // Route X
    while(curX != route.srcX) {

        // Traveling W -> E
        if(curX < route.srcX) {
            curX++;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_WEST);
        }
        // Traveling E -> W
        else {
            curX--;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_EAST);
        }
    }

    // Route Y
    while(curY != route.srcY) {

        // Traveling N -> S
        if(curY < route.srcY) {
            curY++;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_NORTH);
        }
        // Traveling S -> N
        else {
            curY--;
            stepCycle = step(stepCycle, curX, curY, MESH_PORT_SOUTH);
        }
    }

    return (uint64_t)stepCycle;
}
