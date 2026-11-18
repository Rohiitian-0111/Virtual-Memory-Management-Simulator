#include "vm.hpp"

#include <cmath>
#include <stdexcept>

namespace vm {

double Stats::tlbHitRate() const
{
    return accesses ? 100.0 * tlbHits / accesses : 0;
}


double Stats::faultRate() const
{
    return accesses ? 100.0 * pageFaults / accesses : 0;
}


double Stats::amat() const
{
    return accesses ? totalNs / accesses : 0;
}


TLB::TLB(size_t capacity)
    : capacity_(capacity)
{
}


std::optional<int> TLB::lookup(int page)
{
    auto it = map_.find(page);
    if (it == map_.end()) {
        return std::nullopt;
    }

    order_.erase(it->second.it);
    order_.push_front(page);
    it->second.it = order_.begin();
    return it->second.frame;
}


void TLB::put(int page, int frame)
{
    if (capacity_ == 0) {
        return;
    }

    auto it = map_.find(page);
    if (it != map_.end()) {
        order_.erase(it->second.it);
        map_.erase(it);
    } else if (map_.size() >= capacity_) {
        int victim = order_.back();
        order_.pop_back();
        map_.erase(victim);
    }

    order_.push_front(page);
    map_[page] = {frame, order_.begin()};
}


void TLB::invalidate(int page)
{
    auto it = map_.find(page);
    if (it == map_.end()) {
        return;
    }

    order_.erase(it->second.it);
    map_.erase(it);
}


std::vector<std::pair<int, int>> TLB::entries() const
{
    std::vector<std::pair<int, int>> result;
    result.reserve(map_.size());

    for (int page : order_) {
        result.emplace_back(page, map_.at(page).frame);
    }

    return result;
}


VirtualMemoryManager::VirtualMemoryManager(Config cfg)
    : cfg_(std::move(cfg))
    , tlb_(cfg_.tlbSize)
{
    if (cfg_.pageSize == 0 || (cfg_.pageSize & (cfg_.pageSize - 1)) != 0) {
        throw std::invalid_argument("page size must be power of two");
    }

    uint64_t pageCount = (1ull << 16) / cfg_.pageSize;
    if (pageCount == 0 || cfg_.frames == 0) {
        throw std::invalid_argument("invalid configuration");
    }

    pt_.resize(pageCount);
    frameToPage_.assign(cfg_.frames, -1);
    memory_.assign(cfg_.frames, std::vector<uint8_t>(cfg_.pageSize));
}


uint8_t VirtualMemoryManager::backingByte(uint32_t page, uint32_t offset) const
{
    return static_cast<uint8_t>((page * 31u + offset * 17u + 13u) & 0xFFu);
}


int VirtualMemoryManager::chooseVictim()
{
    for (size_t frame = 0; frame < frameToPage_.size(); ++frame) {
        if (frameToPage_[frame] == -1) {
            return static_cast<int>(frame);
        }
    }

    if (cfg_.policy == "FIFO") {
        int victim = static_cast<int>(fifoHand_);
        fifoHand_ = (fifoHand_ + 1) % frameToPage_.size();
        return victim;
    }

    if (cfg_.policy == "CLOCK") {
        while (true) {
            int page = frameToPage_[clockHand_];
            if (!pt_[page].referenced) {
                int victim = static_cast<int>(clockHand_);
                clockHand_ = (clockHand_ + 1) % frameToPage_.size();
                return victim;
            }

            pt_[page].referenced = false;
            clockHand_ = (clockHand_ + 1) % frameToPage_.size();
        }
    }

    int bestFrame = 0;
    for (size_t frame = 1; frame < frameToPage_.size(); ++frame) {
        int candidatePage = frameToPage_[frame];
        int bestPage = frameToPage_[bestFrame];
        if (pt_[candidatePage].lastUsed < pt_[bestPage].lastUsed) {
            bestFrame = static_cast<int>(frame);
        }
    }

    return bestFrame;
}


int VirtualMemoryManager::loadPage(int page, AccessResult& result)
{
    int frame = chooseVictim();
    int evictedPage = frameToPage_[frame];

    if (evictedPage != -1) {
        pt_[evictedPage].valid = false;
        pt_[evictedPage].frame = -1;
        tlb_.invalidate(evictedPage);
        result.evictedPage = evictedPage;
    }

    for (uint32_t offset = 0; offset < cfg_.pageSize; ++offset) {
        memory_[frame][offset] = backingByte(page, offset);
    }

    frameToPage_[frame] = page;
    pt_[page] = {frame, true, true, tick_, tick_};
    return frame;
}


AccessResult VirtualMemoryManager::access(uint32_t address)
{
    if (address > 0xFFFFu) {
        throw std::out_of_range("address must be 0..65535");
    }

    ++tick_;
    ++stats_.accesses;

    AccessResult result;
    result.virtualAddress = address;

    uint32_t pageBits = static_cast<uint32_t>(std::log2(cfg_.pageSize));
    result.page = address >> pageBits;
    result.offset = address & (cfg_.pageSize - 1);

    auto hitFrame = tlb_.lookup(static_cast<int>(result.page));
    if (hitFrame) {
        result.tlbHit = true;
        ++stats_.tlbHits;
        result.frame = *hitFrame;
        stats_.totalNs += cfg_.tlbNs + cfg_.memoryNs;
    } else {
        ++stats_.tlbMisses;
        stats_.totalNs += cfg_.tlbNs + cfg_.memoryNs;

        if (!pt_[result.page].valid) {
            result.pageFault = true;
            ++stats_.pageFaults;
            stats_.totalNs += cfg_.faultNs;
            result.frame = loadPage(static_cast<int>(result.page), result);
        } else {
            result.frame = pt_[result.page].frame;
        }

        tlb_.put(static_cast<int>(result.page), result.frame);
        stats_.totalNs += cfg_.memoryNs;
    }

    pt_[result.page].referenced = true;
    pt_[result.page].lastUsed = tick_;

    result.physicalAddress = static_cast<uint32_t>(result.frame) * cfg_.pageSize + result.offset;
    result.value = memory_[result.frame][result.offset];

    return result;
}

} // namespace vm
