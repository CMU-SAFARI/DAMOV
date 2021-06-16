#ifndef LOCALITY_H_
#define LOCALITY_H_

#include <map>
#include <list>
#include <vector>
#include <set>
class locality{
 public:
    locality();
    void push_address(uint64_t address, uint32_t size);
    void calculate_locality();
    float get_temporal_locality();
    float get_spatial_locality();

    struct request{
        uint64_t address;
        uint32_t size;
    };
    
 private:
  std::map<uint64_t, uint64_t> t_histogram;
  std::map<uint64_t, uint64_t> s_histogram;
  std::map<uint64_t, uint64_t> last_access;
  std::list<uint64_t> past_32;
  std::map<long, int> verify_to_remove_address;

  std::vector<request> requests;

  long counter = 0;
  unsigned max_instructions = 200000000;
  
  float spatial_locality_score = 0;
  uint64_t addr_id = 0;
  uint64_t stride_access = 0;

  float temporal_locality_score = 0;
  uint64_t mem_accesses = 0;
};
#endif  // LOCALITY_H_
