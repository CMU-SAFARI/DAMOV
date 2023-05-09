import os
import csv
import sys

if(len(sys.argv) < 2):
    print("Usage: python script.py path-to-zsim.out-file")
    exit(1)

tmp = sys.argv[1]
# ====================== CPU Metrics  ======================
instructions = 0
cycles = 0

ipc = 0.0
cycles_list = []
# ====================== Cache Metrics   ======================
l3_misses = 0
l2_misses = 0
l1_misses = 0

l3_hits = 0
l2_hits = 0
l1_hits = 0

l3_miss_rate = 0.0
l2_miss_rate = 0.0
l1_miss_rate = 0.0
l3_miss_rate_avg = 0.0

l3_mpki = 0.0
l2_mpki = 0.0
l1_mpki = 0.0

l1d = False
l2d = False
l3d = False

# ====================== Other Metrics    ======================
lfmr = 0.0

core_id = 0
label = ""
with open(tmp, "r") as ins:
    for line in ins:
        try:

            # ====================== CPU Metrics  ======================
            if(line.find("instrs: ")!=-1):
                label = "instrs"
                instructions += int(line.split()[1])
                
            if(line.find("Simulated unhalted cycles")!= -1):
                label = "cycles"
                cycles_list.append(int(line.split()[1]))
                

            # ====================== Cache Metrics  ======================
            if(line.find("l1d:")!=-1):
                l1d = True
                l2d = False
                l3d = False
            if(line.find("l2: # Cache stats")!=-1):
                l1d = False
                l2d = True
                l3d = False
            if(line.find("l3: # Cache stats")!=-1):
                l1d = False
                l2d = False
                l3d = True
            if(line.find("sched: # Scheduler stats")!=-1):
                l1d = False
                l2d = False
                l3d = False


            if(l1d == True):
                if (line.find(": # Filter cache stats")!=-1):
                    label = "l1d"
                    tmp2 = line.split(":")[0]
                    tmp2 = tmp2.replace(":","")
                    core_id = int(tmp2.split("-")[1])
                    
                if(line.find("# GETS hits")!=-1 or line.find("# GETX hits")!=-1):
                    label = "l1d+hits"
                    l1_hits += int(line.split()[1])

                if(line.find("# GETS misses")!=-1 or line.find("# GETX I->M misses")!=-1):
                    label = "l1d+misses"
                    l1_misses += int(line.split()[1])
                    
            if(l2d == True):
                if(line.find("hGETS:")!=-1 or line.find("hGETX:")!=-1):
                    label = "l2d+hits"
                    l2_hits += int(line.split()[1])
                    

                if(line.find("# GETS misses")!=-1):
                    label = "l2d+misses"
                    l2_misses += int(line.split()[1])
                    
            if(l3d == True):
                if(line.find("hGETS:")!=-1 or line.find("hGETX:")!=-1):
                    label = "l3d+hits"
                    l3_hits += int(line.split()[1])
                    
                
                if(line.find("# GETS misses")!=-1 or line.find("# GETX I->M misses")!=-1):
                    label = "l3d+misses"
                    l3_misses += int(line.split()[1])
                    

        except:
            print "Couldn't read some stat. Check label: " + label

# ====================== CPU Metrics  ======================
if(len(cycles_list)!=0):
    cycles = max(cycles_list)

try:
    ipc = float(instructions)/float(cycles)
except:
    ipc = 0.0
       
# ====================== Cache Metrics  ======================

try:
    l3_miss_rate = (l3_misses/float((l3_misses+l3_hits)))*100.0
except:
    l3_miss_rate = 0.0

try:
    l2_miss_rate = (l2_misses/float((l2_misses+l2_hits)))*100.0
except:
    l2_miss_rate = 0.0

try:
    l1_miss_rate = (l1_misses/float((l1_misses+l1_hits)))*100.0
except:
    l1_miss_rate = 0.0


try:
    l3_mpki = l3_misses/float((instructions/1000.0))
    l2_mpki = l2_misses/float((instructions/1000.0))
    l1_mpki = l1_misses/float((instructions/1000.0))
except:
    l3_mpki = 0.0
    l2_mpki = 0.0
    l1_mpki = 0.0

# ====================== Other Metrics  ======================
if(l1_misses):
    lfmr = float(l3_misses/float(l1_misses))
else:
    lfmr = 0

print("\n ------------------ Summary ------------------------")
print("Instructions: " + str(instructions))
print("Cycles: " + str(cycles))
print("IPC: " + str(ipc))
print("L3 Miss Rate (%): " + str(l3_miss_rate))
print("L2 Miss Rate (%): " + str(l2_miss_rate))
print("L1 Miss Rate (%): " + str(l1_miss_rate))
print("L3 MPKI: " + str(l3_mpki))
print("LFMR: " + str(lfmr))
print("")
