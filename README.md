
# DAMOV: A New Methodology and Benchmark Suite for Evaluating Data Movement Bottlenecks
DAMOV is a benchmark suite and a methodical framework targeting the study of data movement bottlenecks in modern applications. It is intended to study new architectures, such as near-data processing. 

The DAMOV benchmark suite is the first open-source benchmark suite for main memory data movement-related studies, based on our systematic characterization methodology. This suite consists of 144 functions representing different sources of data movement bottlenecks and can be used as a baseline benchmark set for future data-movement mitigation research. The applications in the DAMOV benchmark suite belong to popular benchmark suites, including [BWA](https://github.com/lh3/bwa), [Chai](https://chai-benchmarks.github.io/), [Darknet](https://github.com/pjreddie/darknet), [GASE](https://github.com/nahmedraja/GASE), [Hardware Effects](https://github.com/Kobzol/hardware-effects), [Hashjoin](https://systems.ethz.ch/research/data-processing-on-modern-hardware/projects/parallel-and-distributed-joins.html), [HPCC](https://icl.utk.edu/hpcc/), [HPCG](https://www.hpcg-benchmark.org/), [Ligra](http://jshun.github.io/ligra/), [PARSEC](https://parsec.cs.princeton.edu/), [Parboil](http://impact.crhc.illinois.edu/parboil/parboil.aspx), [PolyBench](http://web.cse.ohio-state.edu/~pouchet.2/software/polybench/), [Phoenix](https://github.com/kozyraki/phoenix), [Rodinia](http://www.cs.virginia.edu/rodinia/doku.php), [SPLASH-2](https://github.com/staceyson/splash2), [STREAM](https://github.com/jeffhammond/STREAM).

The DAMOV framework is based on two widely-known simulators: [ZSim](https://github.com/s5z/zsim/) and [Ramulator](https://github.com/CMU-SAFARI/ramulator).  We consider a computing system that includes host CPU cores and PIM cores. The PIM cores are placed in the logic layer of a 3D-stacked memory (Ramulator's HMC model). With this simulation framework, we can simulate host CPU cores and general-purpose PIM cores to compare both for an application or parts of it.


## Citation
Please cite the following preliminary version of our paper if you find this repository useful:

Geraldo F. Oliveira, Juan Gómez-Luna, Lois Orosa, Saugata Ghose, Nandita Vijaykumar, Ivan Fernandez, Mohammad Sadrosadati, Onur Mutlu, "[DAMOV: A New Methodology and Benchmark Suite for Evaluating Data Movement Bottlenecks](https://arxiv.org/pdf/2105.03725.pdf)". arXiv:2105.03725 [cs.AR], 2021.

Bibtex entry for citation:

```
@article{oliveira2021damov,
  title={{DAMOV: A New Methodology and Benchmark Suite for Evaluating Data Movement Bottlenecks}},
  author={Oliveira, Geraldo F and G{\'o}mez-Luna, Juan and Orosa, Lois and Ghose, Saugata and Vijaykumar, Nandita and Fernandez, Ivan and Sadrosadati, Mohammad and Mutlu, Onur},
  journal={IEEE Access},
  year={2021},
}
```

## Setting up DAMOV
### Repository Structure and Installation
We point out next to the repository structure and some important folders and files.

```
.
+-- README.md
+-- get_workloads.sh
+-- simulator/
|   +-- command_files/
|   +-- ramulator/
|   +-- ramulator-configs/
|   +-- scripts/
|   +-- src/
|   +-- templates/

```

### Step 0: Prerequisites
Our framework requires both ZSim and Ramulator dependencies.
* Ramulator requires a C++11 compiler (e.g., clang++, g++-5).
* ZSim requires gcc >=4.6, pin, scons, libconfig, libhdf5, libelfg0: We provide two scripts `setup.sh` and `compile.sh` under `simulator/scripts` to facilitate ZSim's installation. The first one installs all ZSim's dependencies. The second one compiles ZSim.
* We use [lrztar](https://github.com/ckolivas/lrzip) to compress files.  

### Step 1: Installing the Simulator
To install the simulator:
```
cd simulator
sudo sh ./scripts/setup.sh
sh ./scripts/compile.sh
cd ../
```

### Step 2: Downloading the Workloads
To download the workloads:

```
sh get_workloads.sh
```

The `get_workloads.sh` script will download all workloads. The script stores the workloads under the `./workloads` folder.

In case the `get_workloads.sh` script does not work as expected (e.g., due to the user reaching Mega's maximum download quota), one can get the workloads directly from the following link: https://mega.nz/file/Mz51xJyY#J_ai3_Pl5kVvFETurKmBuMIrOagUK4sadyahOzUYQVE 

Please, note that the workload folder requires around 6 GB of storage.

The `./workloads` folder has the following structure:
```
.
+-- workloads/
|   +-- Darknet/
|   +-- GASE-master/
|   +-- PolyBench-ACC/
|   +-- STREAM/
|   +-- bwa/
|   +-- chai-cpu/
|   +-- hardware-effects/
|   +-- hpcc/
|   +-- hpcg/
|   +-- ligra/
|   +-- multicore-hashjoins-0.1/
|   +-- parboil/
|   +-- parsec-3.0/
|   +-- phoenix/
|   +-- rodinia_3.1/

```


## The DAMOV Benchmark Suite
The DAMOV benchmark suite constitutes a set of  144 functions that span across 74 different applications, belonging to 16 different widely-used benchmark suites or frameworks.

Each application is instrumented to delimiter one or more functions of interest (i.e., memory-bound functions). We provide a set of scripts that set up each application in the benchmark suite.

### Application's Dependencies
Please, check each workload's README file for more information regarding its dependencies.

### Application’s Compilation
To aid the compilation of the applications, we provide helping scripts inside each's application folder. The scripts are called `compile.py`. The script (1) compiles the applications,  (2) decompresses the dataset of each application, and  (3) sets their expected file names as defined in the simulator's command files (please, see below).

To illustrate, to compile the STREAM applications:

```
cd workloads/STREAM/
python compile.py
cd ../../
```


## DAMOV-SIM: The DAMOV Simulation Framework
We build a framework that integrates the ZSim CPU simulator with the Ramulator memory simulator to produce a fast, scalable, and cycle-accurate open-source simulator called **DAMOV-SIM**. We use ZSim to simulate the core microarchitecture, cache hierarchy, coherence protocol, and prefetchers. We use Ramulator to simulate the DRAM architecture, memory controllers, and memory accesses. To compute spatial and temporal locality, we modify ZSim to generate a single-thread memory trace for each application, which we use as input for the locality analysis algorithm.

### (1) Simulator Configuration
####  Host and PIM Core Format
ZSim can simulate three types of PIM Cores:

* `OOO`: An out-of-order core.
* `Timing`: A simple 1-issue in-order-like core.
* `Accelerator`: A dataflow accelerator model. The model is designed by issuing at every clock cycle all independent arithmetic instructions in the dataflow graph of a given basic block.

#### ZSim Configuration Files
The user can configure the core model, number of cores, and cache hierarchy structure by creating configuration files. The configuration file will be used as input to ZSim when launching a new simulation.

We provide sample template files under `simulator/templates` for different Host and PIM systems.  These template files are:

* `template_host_nuca_1_core.cfg` :  Defines a host system with a single OOO core,  private L1/L2 caches, and shared NUCA L3 cache.     
* `template_host_nuca.cfg`:  Defines a host system with multiple OOO cores,  private L1/L2 caches, and shared NUCA L3 cache.     
* `template_host_nuca_1_core_inorder.cfg` : Defines a host system with a single Timing core,  private L1/L2 caches, and shared NUCA L3 cache.     
* `template_host_nuca_inorder.cfg` :  Defines a host system with multiple Timing cores,  private L1/L2 caches, and shared NUCA L3 cache.     
* `template_host_accelerator.cfg`: Defines a host system with multiple Accelerator cores,  private L1/L2 caches, and shared L3 cache of fixed size.
*  `template_host_inorder.cfg`:  Defines a host system with multiple Timing cores,  private L1/L2 caches, and shared L3 cache of fixed size.
*  `template_host_ooo.cfg` : Defines a host system with multiple OOO cores,  private L1/L2 caches, and shared L3 cache of fixed size.
* `template_host_prefetch_accelerator.cfg`:  Defines a host system with multiple Accelerator cores,  private L1/L2 caches,  L2 prefetcher, and shared L3 cache of fixed size.
* `template_host_prefetch_inorder.cfg`: Defines a host system with multiple Timing cores,  private L1/L2 caches,  L2 prefetcher, and shared L3 cache of fixed size.
* `template_host_prefetch_ooo.cfg` :  Defines a host system with multiple OOO cores,  private L1/L2 caches,  L2 prefetcher, and shared L3 cache of fixed size.
* `template_pim_accelerator.cfg`: Defines a PIM system with multiple Accelerator cores and private L1 caches.
* `template_pim_inorder.cfg`: Defines a PIM system with multiple Timing cores and private L1 caches.
* `template_pim_ooo.cfg`:  Defines a PIM system with multiple OOO cores and private L1 caches.

#### Generating ZSim Configuration Files
The script under `simulator/scripts/generate_config_files.py` can automatically generate configuration files for a given command file. Command files are used to specify the path to the application binary of interest and its input commands. A list of command files for the workloads under `workloads/` can be found at `simulator/command_files`. To automatically generate configuration files for a given benchmark (STREAM in the example below), one can execute the following command:

```
python scripts/generate_config_files.py command_files/stream_cf
```

The script uses the template files available under `simulator/templates/` to generate the appropriate configuration files. The user needs to modify the script to point to the path of the workloads folder (i.e., PIM_ROOT flag) and the path of the simulator folder (i.e., ROOT flag). You can modify the script also to generate configuration files for different core models by changing the core type when calling the `create_*_configs()` function.

The script stores the generated configuration files under `simulator/config_files`.

### (2) Running an Application from DAMOV
We illustrate how to run an application from our benchmark suite using the `STREAM Add` application as an example.  To execution a host simulation of the `STREAM Add` application, running in a system with four OOO cores:

```
./build/opt/zsim config_files/host_ooo/no_prefetch/stream/4/Add_Add.cfg
```
 The output of the simulation will be stored under `zsim_stats/pim_ooo/4/stream_Add_Add.*`.  

To execution a PIM simulation of the STREAM Add application, running in a system with four OOO cores:

```
./build/opt/zsim config_files/pim_ooo/stream/4/Add_Add.cfg
```

The output of the simulation will be stored under `zsim_stats/pim_ooo/4/stream_Add_Add.*`.  

The script under `simulator/scripts/generate_config_files.py` can parse some useful statistics from a simulation.

For example,  the user can collect the IPC of the execution of the host simulation of the STREAM Add application, running in a system with four OOO cores by executing:

```
python scripts/get_stats_per_app.py zsim_stats/host_ooo/no_prefetch/4/stream_Add_Add.zsim.out

Output:
 ------------------ Summary ------------------------
Instructions: 1000002055
Cycles: 450355583
IPC: 2.22047220629
L3 Miss Rate (%): 99.9991935477
L2 Miss Rate (%): 100.0
L1 Miss Rate (%): 73.563163442
L3 MPKI: 23.4357438395
LFMR: 0.999992703522
```

Likewise,  the user can collect the IPC of the execution of the PIM simulation of the STREAM Add application, running in a system with four OOO cores by executing:

```
python scripts/get_stats_per_app.py zsim_stats/pim_ooo/4/stream_Add_Add.zsim.out

Output:
 ------------------ Summary ------------------------
Instructions: 1000009100
Cycles: 284225084
IPC: 3.5183703209
L3 Miss Rate (%): 0.0
L2 Miss Rate (%): 0.0
L1 Miss Rate (%): 73.563253602
L3 MPKI: 0.0
LFMR: 0.0
```

In this way, the speedup the PIM system provides compared to the host system for this particular application is of  `3.5183703209/ 2.22047220629 = 1.58451446`.

Please, note that the simulation framework does not currently support concurrent execution on host and PIM cores.

### (3) Instrumenting and Simulating New Applications
There are three steps to run a simulation with ZSim:

1. Instrument the code with the hooks provided in `workloads/zsim_hooks.h`.
2. Create configuration files for ZSim.
3. Run.

Next, we describe the three steps in detail:

1. First, we identify the application's hotspot. We refer to it as the `offload` region, i.e., the region of code that will run in the PIM cores. We instrument the application by including the following code:

```cpp
#include "zsim_hooks.h"
foo(){
    /*
    * zsim_roi_begin() marks the beginning of the region of interest (ROI).
    * It must be included in a serial part of the code.
    */
	zsim_roi_begin();
	zsim_PIM_function_begin(); // Indicates the beginning of the code to simulate (hotspot).
	...
	zsim_PIM_function_end(); // Indicates the end of the code to simulate.
    /*
    * zsim_roi_end() marks the end of the ROI.
    * It must be included in a serial part of the code.
    */
	zsim_roi_end();
}
```

2. Second, we create the configuration files to execute the application using ZSim. Sample configuration files are provided under `simulator/config_files/`. Please, check those files to understand how to configure the number of cores, number of caches and their sizes, and number prefetchers. Next, we describe other important knobs that can be changed in the configuration files:

* `pimMode=true|false`: When set to `true`, ZSim will simulate a memory model with shorter memory latency and higher memory bandwidth. When set to `false`, it will simulate a regular memory device.
* `max_offload_instrs`: Maximum number of offload instructions to execute.

3. Third, we run ZSim:
```
./build/opt/zsim configuration_file.cfg
```

## Getting Help
If you have any suggestions for improvement, please contact geraldo dot deoliveira at safari dot ethz dot ch.
If you find any bugs or have further questions or requests, please post an issue at the [issue page](https://github.com/CMU-SAFARI/damov/issues).

## Acknowledgments
We acknowledge support from the SAFARI Research Group’s industrial partners, especially ASML, Facebook, Google, Huawei, Intel, Microsoft, VMware, and the Semiconductor Research Corporation.
