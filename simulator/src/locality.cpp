#include "locality.h"
#include <math.h>
#include <cstdlib>
#include <iostream>

using namespace std;
locality::locality(){
  for (int i=0; i < 21; i++) {
    uint64_t value = pow(2, i);
    t_histogram[value] = 0;
    s_histogram[value] = 0;
  }
}


void locality::push_address(uint64_t addr, uint32_t size){

    if(counter >= max_instructions) return;
    else counter++;


    if(verify_to_remove_address.find(addr)!=verify_to_remove_address.end()){
        verify_to_remove_address[addr]++;
    }
    else{
        verify_to_remove_address[addr]=1;
    }

    request new_rqst;
    new_rqst.address = addr;
    new_rqst.size = size;

    requests.push_back(new_rqst);
}


void locality::calculate_locality(){

  /* Remove stack addresses from address stream.
  * Empirically, we observe that stack addresses appear more than 2^21 times in the address stream.
  * For more information why removing stack addresses is important to calculate locality, check WIICA,ISPASS'13 paper. 
  */
   std::set<long> to_remove;
    for (std::map<long,int>::iterator it=verify_to_remove_address.begin(); it!=verify_to_remove_address.end(); ++it){
        int index = int(ceil(log2(it->second)));
        if (index >= 21) to_remove.insert(it->first);
    }
    verify_to_remove_address.clear();

  for(int i = 0; i < requests.size(); i++){
    uint64_t addr = requests[i].address;
    uint32_t size = requests[i].size;

    if(to_remove.find(addr)!=to_remove.end())
        continue;

    addr = (addr >> (int)log2(size));
    addr_id += 1;
    mem_accesses += 1;

    // Reuse Distance
    if (last_access.find(addr) != last_access.end()) {
        uint64_t stride = addr_id - last_access[addr];
        if (stride > 1048576)
            stride = 1048576;
        t_histogram[pow(2, ceil(log2(stride)))] += 1;
    }
    last_access[addr] = addr_id;

    // Histogram of stride access
    if (past_32.size() >= 32){
        uint64_t stride = 1048576;

        for (auto const& item : past_32) {
            if (abs( ((long)item) - (long(addr))) < stride){
                stride = abs(long(item) - long(addr));
            }
        }
        if (stride != 0) {
            s_histogram[pow(2, ceil(log2(stride)))] += 1;
            stride_access += 1;
        }
        past_32.pop_front();
    }
    past_32.push_back(addr);
  }

  to_remove.clear();
}

float locality::get_spatial_locality(){
  float percent;
  for (int i = 0; i < 21; i++){
    if (stride_access == 0){
      return 0.0;
    }

    percent = s_histogram[pow(2, i)] * 1.0 / stride_access;
    spatial_locality_score += percent * 1.0 / pow(2, i);
  }

  return spatial_locality_score;
}

float locality::get_temporal_locality(){
  float percent;
  if(mem_accesses == 0 )
    return temporal_locality_score = 0;

  for (int i = 0; i < 21; i++){
    percent = t_histogram[pow(2, i)] * 1.0 / mem_accesses;
    temporal_locality_score += percent * 1.0 * (21 - i) / 21;
  }

  return temporal_locality_score;
}
