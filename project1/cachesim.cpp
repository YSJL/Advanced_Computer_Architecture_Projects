#include "cachesim.hpp"
#include <cstdio>
#include <iostream>
#include <deque>

/**
 * The use of virtually indexed physically tagged caches limits
 *      the total number of sets you can have.
 * If the user selected configuration is invalid for VIPT
 *      Update config->s to reflect the minimum value for S (log2 number of ways)
 * If the user selected configuration is valid
 *      do not modify it.
 * TODO: You're responsible for completing this routine
*/

/*
 * What do we need?
 * TODO: Not enough hits, figure out why
 * TODO: Writeback not working figure out why
 *
*/

//dequeue, most push back,least pop front
//list<uint64_t*> LRU;
uint64_t C;
uint64_t B;
uint64_t S;
uint64_t numBlocks;
uint64_t numOffset;
uint64_t numSet;
uint64_t numIndex;

const uint64_t VALID = 2;
const uint64_t DIRTY = 1;

struct tagBlock
{
    uint64_t tag;
    uint64_t valid;
    uint64_t dirty;
    uint64_t* dataBlocks;
    //When using LRU only for tagBlock[0][index]
    //Use set number for LRU and evict least used
    std::deque<uint64_t> LRU;
};
struct tagBlock** cacheBlocks;

void legalize_s(sim_config_t *config) {
  //Checks if setup is for Physically Indexed L1 Cache
  config->vipt = false; //TODO: ERASE THIS
  if (config->vipt == false) {
    //Legalize c and b to match requirements
    if (config->c < 9) {
      config->c = 9;
    } else if (config->c > 18) {
      config->c = 18;
    }
    if (config->b < 4) {
      config->b = 4;
    } else if (config->b > 7) {
      config->b = 7;
    }
  }
}

/**
 * Subroutine for initializing the cache simulator. You many add and initialize any global or heap
 * variables as needed.
 * TODO: You're responsible for completing this routine
 */

void sim_setup(sim_config_t *config) {
  //Checks if setup is for Physically Indexed L1 Cache
  if (config->vipt == false) {
    //Basic setup
    C = config->c;
    B = config->b;
    S = config->s;
    numBlocks = 1 << (config->c - config->b);
    numOffset = 1 << config->b;
    numSet = 1 << config->s;
    numIndex = 1 << (64 - ((config->c - config->s) + config->b));

    if (C - B - S == 0) {
      //Direct
      numIndex = numSet;
      numSet = 1;
    } else if (S == 0) {
      //Fully Associative
      numIndex = 1;
      numSet = numBlocks;
    }

    cacheBlocks = new tagBlock*[numSet]();
    for (uint64_t sets = 0; sets < numSet; sets++) {
      cacheBlocks[sets] = new tagBlock[numIndex]();
      for (uint64_t index = 0; index < numIndex; index++) {
        cacheBlocks[sets][index].tag = 0;
        cacheBlocks[sets][index].valid = 0;
        cacheBlocks[sets][index].dirty = 0;
        cacheBlocks[sets][index].dataBlocks = new uint64_t[numBlocks]();
      }
    }

  }
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 * TODO: You're responsible for completing this routine
 */
void sim_access(char rw, uint64_t addr, sim_stats_t* stats) {
  stats->accesses_l1++;
  if (hit(rw, addr, stats)) {
    if (rw == READ) {
      //read hit
      stats->reads++;
    } else {
      //write hit
      writeback(rw, addr, stats);
      stats->writes++;
    }
  } else {
    //
    uint64_t set = pullblockSet(rw, addr, stats);

    cacheBlocks[set][getBlockIndex(addr)].tag = getBlockTag(addr);
    cacheBlocks[set][getBlockIndex(addr)].valid = 1;
    cacheBlocks[set][getBlockIndex(addr)].dataBlocks[getBlockOffset(addr)] = addr;
    if (rw == READ) {
    //read miss
    stats->reads++;
    } else {
    //write miss
    writeback(rw,addr,stats);
    stats->writes++;
    }
  }
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * TODO: You're responsible for completing this routine
 */
void sim_finish(sim_stats_t *stats) {
  for (uint64_t sets = 0; sets < numSet; sets++) {
    for (uint64_t index = 0; index < numIndex; index++) {
//      cacheBlocks[sets][index].tag = 0;
//      cacheBlocks[sets][index].valid = 0;
//      cacheBlocks[sets][index].dirty = 0;
      delete[] cacheBlocks[sets][index].dataBlocks;
      cacheBlocks[sets][index].LRU = std::deque<uint64_t>();
    }
    delete[] cacheBlocks[sets];
  }
  delete[] cacheBlocks;
  stats->hit_ratio_l1 = (double) stats->hits_l1 / (double) stats->accesses_l1;
  stats->miss_ratio_l1 = (double) stats->misses_l1 / (double) stats->accesses_l1;
}

/**
 * Method to check if address is a hit
 */
bool hit(char rw, uint64_t addr, sim_stats_t* stats) {
  //printf("In Hit? Method\n");
  /*printf("addr:%li, tag: %li, dataoffset: %li\n",
    addr, getBlockTag(addr), getBlockOffset(addr));*/
  for (uint64_t i = 0; i < numSet; i++) {
    //printf("HitM LRU size: %lu\n", cacheBlocks[0][getBlockIndex(addr)].LRU.size());
    stats->array_lookups_l1++;
    stats->tag_compares_l1++;
    if (cacheBlocks[i][getBlockIndex(addr)].valid == 1 &&
      cacheBlocks[i][getBlockIndex(addr)].tag == getBlockTag(addr)) {
      //Update LRU
      LRUupdate(i, addr, stats);
      stats->hits_l1++;
      return true;
    }
  }
  stats->misses_l1++;
  return false;
}

/**
 * Method to writeback a Physically Indexed Cache
 */
void writeback(char rw, uint64_t addr, sim_stats_t* stats) {
   uint64_t hitSet = cacheBlocks[0][getBlockIndex(addr)].LRU.back();
   cacheBlocks[hitSet][getBlockIndex(addr)].dirty = 1;
   cacheBlocks[hitSet][getBlockIndex(addr)].dataBlocks[getBlockOffset(addr)] = addr;
}

/**
 * Method to check if cache is full for the set, and evict if so
 */
uint64_t pullblockSet(char rw, uint64_t addr, sim_stats_t* stats) {
  uint64_t evictSet = -1;
  if (cacheBlocks[0][getBlockIndex(addr)].LRU.size() >= numSet) {
    evictSet = cacheBlocks[0][getBlockIndex(addr)].LRU.front();
    if (cacheBlocks[evictSet][getBlockIndex(addr)].dirty == 1) {
      //Update Memory due to dirty cache evicted
      stats->writebacks_l1++;
    }
    cacheBlocks[evictSet][getBlockIndex(addr)].tag = 0;
    cacheBlocks[evictSet][getBlockIndex(addr)].valid = 0;
    cacheBlocks[evictSet][getBlockIndex(addr)].dirty = 0;
    cacheBlocks[0][getBlockIndex(addr)].LRU.pop_front();
  } else {
    evictSet = cacheBlocks[0][getBlockIndex(addr)].LRU.size();
  }

  cacheBlocks[evictSet][getBlockIndex(addr)].tag = getBlockTag(addr);
  cacheBlocks[evictSet][getBlockIndex(addr)].valid = 1;
  cacheBlocks[evictSet][getBlockIndex(addr)].dataBlocks[getBlockOffset(addr)] = addr;

  //LRU update
  LRUupdate(evictSet, addr, stats);

  return evictSet;
}

/**
 * Method to find the way index for LRU
 */
uint64_t findLRUIndex(uint64_t set, uint64_t addr, sim_stats_t* stats) {
  for (uint64_t i = 0; i < cacheBlocks[0][getBlockIndex(addr)].LRU.size(); i++) {
    if (cacheBlocks[0][getBlockIndex(addr)].LRU.at(i) == set) {
      return i;
    }
  }
  return 0;
}

/**
 * Method to update LRU
 */
void LRUupdate(uint64_t set, uint64_t addr, sim_stats_t* stats) {
  //printf("LRUupdate before LRU size: %lu\n", cacheBlocks[0][getBlockIndex(addr)].LRU.size());
  if (cacheBlocks[0][getBlockIndex(addr)].LRU.size() >= numSet) {
    cacheBlocks[0][getBlockIndex(addr)].LRU.erase(cacheBlocks[0][getBlockIndex(addr)].LRU.begin() + findLRUIndex(set, addr, stats));
  }
  cacheBlocks[0][getBlockIndex(addr)].LRU.push_back(set);
  //printf("LRUupdate after LRU size: %lu\n", cacheBlocks[0][getBlockIndex(addr)].LRU.size());
}

uint64_t getBlockTag(uint64_t addr) {
  return (addr / numBlocks);
}

uint64_t getBlockOffset(uint64_t addr) {
  return addr % numOffset;
}

uint64_t getBlockIndex(uint64_t addr) {
  if (numIndex == 1) {
    return 0;
  }
  return getBlockAddr(addr) % numSet;
}

uint64_t getBlockAddr(uint64_t addr) {
  return addr / numOffset;
}