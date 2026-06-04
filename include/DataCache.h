#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CacheConfig {
    bool enabled = false;

    int numSets = 8;
    // The simulator memory is word-addressed; cache blocks are sized in words.
    int blockSizeWords = 4;

    int hitLatency = 1;
    int missPenalty = 10;
};

struct CacheLine {
    // Metadata-only cache line. Data values are not stored independently here;
    // trace/UI block contents are reconstructed from architectural memory.
    bool valid = false;
    bool dirty = false;
    std::uint32_t tag = 0;
};

struct CacheAccessResult {
    bool enabled = false;

    bool hit = false;
    bool miss = false;
    bool writeback = false;
    bool fillRequired = false;
    bool dirtyOnFill = false;

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
    // Simple blocking model: a set with an outstanding fill cannot accept a
    // younger access until the miss latency completes.
    bool canStartAccess(std::uint32_t address) const;
    // Installs valid/tag/dirty metadata for a missed block when the modeled
    // lower-memory fill completes.
    void completeAccess(const CacheAccessResult& result);
    // Used when a wrong-path instruction is flushed before its pending fill
    // reaches completion.
    void cancelAccess(const CacheAccessResult& result);

    const CacheConfig& getConfig() const;
    const std::vector<CacheLine>& getLines() const;
    const CacheStats& getStats() const;

    void reset();

private:
    CacheConfig config;
    std::vector<CacheLine> lines;
    CacheStats stats;

    struct PendingFill {
        // Represents a block currently being fetched from lower memory. The
        // real line remains invalid/old-tagged until completeAccess().
        bool valid = false;
        bool dirtyOnFill = false;
        std::uint32_t tag = 0;
    };

    std::vector<PendingFill> pendingFills;

    struct DecodedAddress {
        std::uint32_t blockAddress = 0;
        std::uint32_t setIndex = 0;
        std::uint32_t tag = 0;
        std::uint32_t blockOffset = 0;
    };

    CacheAccessResult access(std::uint32_t address, bool isStore);
    DecodedAddress decodeAddress(std::uint32_t address) const;
};
