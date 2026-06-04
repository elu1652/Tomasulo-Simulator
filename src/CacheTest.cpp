#include <iostream>
#include "DataCache.h"

int main() {
    CacheConfig config;
    config.enabled = true;
    config.numSets = 8;
    config.blockSizeWords = 16;
    config.hitLatency = 1;
    config.missPenalty = 10;

    DataCache cache(config);

    auto a = cache.load(0);
    auto b = cache.load(4);
    auto c = cache.load(8);
    auto d = cache.load(12);

    std::cout << a.hit << " " << a.miss << " " << a.latency << "\n";
    std::cout << b.hit << " " << b.miss << " " << b.latency << "\n";
    std::cout << c.hit << " " << c.miss << " " << c.latency << "\n";
    std::cout << d.hit << " " << d.miss << " " << d.latency << "\n";

    return 0;
}