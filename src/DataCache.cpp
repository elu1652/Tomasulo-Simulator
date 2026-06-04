#include "DataCache.h"

DataCache::DataCache(const CacheConfig& config)
    : config(config), lines(config.numSets) {}

CacheAccessResult DataCache::load(std::uint32_t address) {
    return access(address, false);
}

CacheAccessResult DataCache::store(std::uint32_t address) {
    return access(address, true);
}

CacheAccessResult DataCache::access(std::uint32_t address, bool isStore) {
    CacheAccessResult result;

    result.enabled = config.enabled;
    result.address = address;
    result.accessType = isStore ? "SD" : "LD";

    if (!config.enabled) {
        result.latency = 0;
        return result;
    }

    std::uint32_t blockAddress = address / config.blockSizeWords;
    std::uint32_t setIndex = blockAddress % config.numSets;
    std::uint32_t tag = blockAddress / config.numSets;
    std::uint32_t blockOffset = address % config.blockSizeWords;

    result.setIndex = setIndex;
    result.tag = tag;
    result.blockOffset = blockOffset;

    CacheLine& line = lines[setIndex];

    bool isHit = line.valid && line.tag == tag;

    stats.accesses++;

    if (isHit) {
        result.hit = true;
        result.miss = false;
        result.latency = config.hitLatency;

        stats.hits++;
    } else {
        result.hit = false;
        result.miss = true;
        result.latency = config.hitLatency + config.missPenalty;

        stats.misses++;

        if (line.valid && line.dirty) {
            result.writeback = true;
            stats.writebacks++;
        }

        line.valid = true;
        line.dirty = false;
        line.tag = tag;
    }

    if (isStore) {
        line.dirty = true;
    }

    stats.totalAccessLatency += result.latency;

    return result;
}

const CacheConfig& DataCache::getConfig() const {
    return config;
}

const std::vector<CacheLine>& DataCache::getLines() const {
    return lines;
}

const CacheStats& DataCache::getStats() const {
    return stats;
}

void DataCache::reset() {
    lines.assign(config.numSets, CacheLine{});
    stats = CacheStats{};
}