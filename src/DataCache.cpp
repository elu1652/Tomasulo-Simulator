#include "DataCache.h"

DataCache::DataCache(const CacheConfig& config)
    : config(config),
      lines(config.numSets),
      pendingFills(config.numSets) {}

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

    // Direct-mapped lookup using word addresses.
    DecodedAddress decoded = decodeAddress(address);

    result.setIndex = decoded.setIndex;
    result.tag = decoded.tag;
    result.blockOffset = decoded.blockOffset;

    CacheLine& line = lines[decoded.setIndex];

    bool isHit = line.valid && line.tag == decoded.tag;

    stats.accesses++;

    if (isHit) {
        // Hits update dirty metadata immediately for stores.
        result.hit = true;
        result.miss = false;
        result.latency = config.hitLatency;

        stats.hits++;
    } else {
        // Misses reserve a pending fill but do not install valid/tag yet.
        // Younger accesses to this set wait until completeAccess().
        result.hit = false;
        result.miss = true;
        result.latency = config.hitLatency + config.missPenalty;

        stats.misses++;

        if (line.valid && line.dirty) {
            result.writeback = true;
            stats.writebacks++;
        }

        result.fillRequired = true;
        result.dirtyOnFill = isStore;

        pendingFills[decoded.setIndex].valid = true;
        pendingFills[decoded.setIndex].tag = decoded.tag;
        pendingFills[decoded.setIndex].dirtyOnFill = isStore;
    }

    if (isStore && isHit) {
        line.dirty = true;
    }

    stats.totalAccessLatency += result.latency;

    return result;
}

bool DataCache::canStartAccess(std::uint32_t address) const {
    if (!config.enabled) {
        return true;
    }

    DecodedAddress decoded = decodeAddress(address);
    return !pendingFills[decoded.setIndex].valid;
}

void DataCache::completeAccess(const CacheAccessResult& result) {
    if (!config.enabled || !result.fillRequired) {
        return;
    }

    // Only now does the fetched block become visible to later accesses.
    CacheLine& line = lines[result.setIndex];
    line.valid = true;
    line.dirty = result.dirtyOnFill;
    line.tag = result.tag;

    pendingFills[result.setIndex] = PendingFill{};
}

void DataCache::cancelAccess(const CacheAccessResult& result) {
    if (!config.enabled || !result.fillRequired) {
        return;
    }

    PendingFill& pendingFill = pendingFills[result.setIndex];

    if (pendingFill.valid && pendingFill.tag == result.tag) {
        pendingFill = PendingFill{};
    }
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
    pendingFills.assign(config.numSets, PendingFill{});
    stats = CacheStats{};
}

DataCache::DecodedAddress DataCache::decodeAddress(std::uint32_t address) const {
    DecodedAddress decoded;

    // Addresses are word indexes: block 0 contains words [0, blockSizeWords).
    decoded.blockAddress = address / config.blockSizeWords;
    decoded.setIndex = decoded.blockAddress % config.numSets;
    decoded.tag = decoded.blockAddress / config.numSets;
    decoded.blockOffset = address % config.blockSizeWords;

    return decoded;
}
