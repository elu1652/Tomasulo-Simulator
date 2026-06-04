#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CacheConfig {
    bool enabled = false;

    int numSets = 8;
    int blockSizeWords = 4;

    int hitLatency = 1;
    int missPenalty = 10;
};

struct CacheLine {
    bool valid = false;
    bool dirty = false;
    std::uint32_t tag = 0;
};

struct CacheAccessResult {
    bool enabled = false;

    bool hit = false;
    bool miss = false;
    bool writeback = false;

    int latency = 0;

    std::uint32_t address = 0;
    std::uint32_t setIndex = 0;
    std::uint32_t tag = 0;
    std::uint32_t blockOffset = 0;

    std::string accessType; // "LD" or "SD"
};

struct CacheStats {
    int accesses = 0;
    int hits = 0;
    int misses = 0;
    int writebacks = 0;
    int totalAccessLatency = 0;

    double hitRate() const {
        return accesses == 0 ? 0.0 : static_cast<double>(hits) / accesses;
    }

    double missRate() const {
        return accesses == 0 ? 0.0 : static_cast<double>(misses) / accesses;
    }

    double averageAccessLatency() const {
        return accesses == 0 ? 0.0 : static_cast<double>(totalAccessLatency) / accesses;
    }
};

class DataCache {
public:
    explicit DataCache(const CacheConfig& config);

    CacheAccessResult load(std::uint32_t address);
    CacheAccessResult store(std::uint32_t address);

    const CacheConfig& getConfig() const;
    const std::vector<CacheLine>& getLines() const;
    const CacheStats& getStats() const;

    void reset();

private:
    CacheConfig config;
    std::vector<CacheLine> lines;
    CacheStats stats;

    CacheAccessResult access(std::uint32_t address, bool isStore);
};