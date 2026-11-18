#pragma once
#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vm {
struct Config { uint32_t pageSize=256, frames=8, tlbSize=4; std::string policy="LRU"; double tlbNs=5, memoryNs=100, faultNs=8000; };
struct PageEntry { int frame=-1; bool valid=false; bool referenced=false; uint64_t lastUsed=0, loadedAt=0; };
struct AccessResult { uint32_t virtualAddress=0,page=0,offset=0; int frame=-1; uint32_t physicalAddress=0; bool tlbHit=false,pageFault=false; int evictedPage=-1; uint8_t value=0; };
struct Stats { uint64_t accesses=0,tlbHits=0,tlbMisses=0,pageFaults=0; double totalNs=0; double tlbHitRate()const; double faultRate()const; double amat()const; };
class TLB {
    size_t capacity_; std::list<int> order_; struct Item{int frame;std::list<int>::iterator it;}; std::unordered_map<int,Item> map_;
public: explicit TLB(size_t cap); std::optional<int> lookup(int page); void put(int page,int frame); void invalidate(int page); std::vector<std::pair<int,int>> entries() const;
};
class VirtualMemoryManager {
    Config cfg_; std::vector<PageEntry> pt_; std::vector<int> frameToPage_; std::vector<std::vector<uint8_t>> memory_; TLB tlb_; Stats stats_; uint64_t tick_=0; size_t fifoHand_=0, clockHand_=0;
    int chooseVictim(); int loadPage(int page, AccessResult& r); uint8_t backingByte(uint32_t page,uint32_t offset) const;
public:
    explicit VirtualMemoryManager(Config cfg); AccessResult access(uint32_t address); const Stats& stats() const{return stats_;} const Config& config()const{return cfg_;}
    std::vector<PageEntry> pageTable()const{return pt_;} std::vector<int> frames()const{return frameToPage_;} std::vector<std::pair<int,int>> tlbEntries()const{return tlb_.entries();}
};
}
