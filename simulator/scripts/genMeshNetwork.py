#!/usr/bin/python

import sys, os;

f = open("network", "w");

numCores = int(sys.argv[1]);
l1iPrefix = "l1i-";
l1dPrefix = "l1d-";
l2Prefix = "l2-";
l3Prefix = "l3-0b";
xDim = int(sys.argv[2]);
yDim = int(sys.argv[2]);
linkLat = 1;
routerLat = 2;
totalLat = linkLat + routerLat;

print("Number of cores: " + sys.argv[1] + " requires a " + sys.argv[2] + "x" + sys.argv[2] + " network")

# 32-core 6x6 mesh with 4 memory controllers, each handling 2 channels

# . . x . . .
# .         .
# .         x
# x         .
# .         .
# . . . x . .

#channelsPerController = 2;
#memControllers=[[2, 0],
#                [5, 2],
#                [0, 3],
#                [3, 5]];

channelsPerController = 1
memControllers = [[0,0]]
added = [];

# Generate core mapping. Mesh stops without memory controllers contain a core.

cores = [];

x = 0;
y = 0;
for i in range(xDim*yDim):
    isMem = False;
    for c in range(len(memControllers)):
        if(x == memControllers[c][0] and y == memControllers[c][1]):
            isMem = True;
    if(not isMem):
        cores.append([x, y]);

    x = x + 1;
    if(x == xDim):
        x = 0;
        y = y + 1;

print(cores);
print(len(cores));

# Write header
f.write(str(xDim)+" "+str(yDim)+" "+str(totalLat)+"\n");

# Write all possible point-to-point interconnect mappings
for i in range(numCores):
    l1i = l1iPrefix+str(i);
    l1d = l1dPrefix+str(i);
    l2 = l2Prefix+str(i);
    l3 = l3Prefix+str(i);
    mem = "mem";
    dist = 0; # latency is already measured in cache
    print "Connecting "+l1i+" to "+l2;
    f.write(l1i+" "+l2+" "+"0 0\n");

    print "Connecting "+l1d+" to "+l2;
    f.write(l1d+" "+l2+" "+"0 0\n");

    # Calculate min distance from L3 bank to mesh edge
    l3x = cores[i][0];
    l3y = cores[i][1];

    channel = 0;
    for mc in memControllers:
        dist = abs(l3x - mc[0])+abs(l3y - mc[1]);
        for c in range(channelsPerController):
            print "Connecting "+l3+" to mem-"+str(channel)+", dist "+str(dist);
            f.write(l3+" mem-"+str(channel)+" 1 "+str(l3x)+" "+str(l3y)+" "+str(mc[0])+" "+str(mc[1])+"\n");
            channel = channel + 1;

    for j in range(numCores):
        l2 = l2Prefix+str(i);
        l3 = l3Prefix+str(j);
        l2x = cores[i][0];
        l2y = cores[i][1];
        l3x = cores[j][0];
        l3y = cores[j][1];
        dist = abs(l2x - l3x)+abs(l2y-l3y);
        lat = dist*(linkLat+routerLat);
        print "Connecting "+l2+" ("+str(l2x)+", "+str(l2y)+") to "+l3+" ("+str(l3x)+", "+str(l3y)+") = "+str(dist)+", "+str(lat)+" cycles";

        if((added.count(l2+l3) == 0) and (added.count(l3+l2) == 0)):
            added.append(l2+l3);
            added.append(l3+l2);
            f.write(l2+" "+l3+" 1 "+str(l2x)+" "+str(l2y)+" "+str(l3x)+" "+str(l3y)+"\n");
        else:
            print "ERROR";
            sys.exit();
