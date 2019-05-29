//
//  main.cpp
//  blocksci-test
//
//  Created by Harry Kalodner on 1/3/17.
//  Copyright © 2017 Harry Kalodner. All rights reserved.
//

#define BLOCKSCI_WITHOUT_SINGLETON

#include <blocksci/blocksci.hpp>
#include <range/v3/view/slice.hpp>
#include <clipp.h>

#include <numeric>
#include <iostream>

using namespace blocksci;

int64_t calculateMaxOutputSingleThreaded(BlockRange &chain);
int64_t calculateMaxOutputMultithreaded(BlockRange &chain);
int64_t calculateMaxInputSingleThreaded(BlockRange &chain);
int64_t calculateMaxInputMultithreaded(BlockRange &chain);
int64_t calculateMaxFeeSingleThreaded(BlockRange &chain);
int64_t calculateMaxFeeMultithreaded(BlockRange &chain);
uint32_t calculateNonzeroLocktimeSingleThreaded(BlockRange &chain);
uint32_t calculateNonzeroLocktimeMultithreaded(BlockRange &chain);

uint32_t calculateNonzeroLocktimeRandom(Blockchain &chain, const std::vector<uint32_t> &indexes);
int64_t calculateMaxFeeRandom(Blockchain &chain, const std::vector<uint32_t> &indexes);

template <typename Func, typename... Args>
auto timeFunc(std::string name, Func func, Args&& ...args) -> decltype(func(args...));

int main(int argc, char * argv[]) {
    bool includeRandom = false;
    std::string configLocation;
    int endBlock = -1;

    auto cli = (
        clipp::value("config file location", configLocation),
        clipp::option("--with-random").set(includeRandom).doc("Include random order benchmarks"),
        clipp::option("-m", "--max-block") & clipp::value("Run benchmark up to the given block", endBlock)
    );
    auto res = parse(argc, argv, cli);
    if (res.any_error()) {
        std::cout << "Invalid command line parameter\n" << clipp::make_man_page(cli, argv[0]);
        return 0;
    }
    Blockchain chain(configLocation);
    
    int startBlock = 0;
    if (endBlock == -1) {
        endBlock = static_cast<int>(chain.size());
    }
    
    std::cout << "Running performance tests up to block " << endBlock << "\n";
    
    auto range = chain[{startBlock, endBlock}];
    
    std::cout << "Heating up cache\n";
    calculateMaxFeeMultithreaded(range);
    std::cout << "Finished heating up cache\n";

    auto maxSize1 = timeFunc("calculateNonzeroLocktimeSingleThreaded", calculateNonzeroLocktimeSingleThreaded, range);
    auto maxSize2 = timeFunc("calculateNonzeroLocktimeMultithreaded", calculateNonzeroLocktimeMultithreaded, range);
    
    auto maxOutput1 = timeFunc("calculateMaxOutputSingleThreaded", calculateMaxOutputSingleThreaded, range);
    auto maxOutput2 = timeFunc("calculateMaxOutputMultithreaded", calculateMaxOutputMultithreaded, range);
    auto maxInput1 = timeFunc("calculateMaxInputSingleThreaded", calculateMaxInputSingleThreaded, range);
    auto maxInput2 = timeFunc("calculateMaxInputMultithreaded", calculateMaxInputMultithreaded, range);
    auto maxFee1 = timeFunc("calculateMaxFeeSingleThreaded", calculateMaxFeeSingleThreaded, range);
    auto maxFee2 = timeFunc("calculateMaxFeeMultithreaded", calculateMaxFeeMultithreaded, range);

    if (includeRandom) {
        uint32_t maxTxNum = chain[endBlock - 1].endTxIndex();
        std::vector<uint32_t> indexes(maxTxNum);
        std::iota(indexes.begin(), indexes.end(), 0);
        std::random_shuffle(indexes.begin(), indexes.end());

        timeFunc("calculateMaxFeeRandom", calculateMaxFeeRandom, chain, indexes);
        timeFunc("calculateNonzeroLocktimeRandom", calculateNonzeroLocktimeRandom, chain, indexes);
    }
    
    std::cout << "Nonzero Locktime = (" << maxSize1 << ", " << maxSize2 << ")\n";
    std::cout << "Max Output = (" << maxOutput1 << ", " << maxOutput2 << ")\n";
    std::cout << "Max Input = (" << maxInput1 << ", " << maxInput2 << ")\n";
    std::cout << "Max Fee = (" << maxFee1 << ", " << maxFee2 << ")\n";
    return 0;
}

int64_t calculateMaxOutputSingleThreaded(BlockRange &chain) {
    int64_t curMax = 0;
    for (auto block : chain) {
        RANGES_FOR(auto tx, block) {
            for (auto output : tx.outputs()) {
                curMax = std::max(curMax, output.getValue());
            }
        }
    }
    return curMax;
}

int64_t calculateMaxOutputMultithreaded(BlockRange &chain) {
    auto extract = [](const Transaction &tx) {
        int64_t maxValue = 0;
        for(auto output : tx.outputs()) {
            maxValue = std::max(output.getValue(), maxValue);
        }
        return maxValue;
    };
    
    auto combine = [](int64_t &a, int64_t &b) -> int64_t & { a = std::max(a,b); return a; };
    
    return chain.mapReduce<int64_t>(extract, combine);
}

int64_t calculateMaxInputSingleThreaded(BlockRange &chain) {
    int64_t curMax = 0;
    for (auto block : chain) {
        RANGES_FOR(auto tx, block) {
            for (auto input : tx.inputs()) {
                curMax = std::max(curMax, input.getValue());
            }
        }
    }
    return curMax;
}

int64_t calculateMaxInputMultithreaded(BlockRange &chain) {
    auto extract = [&](const Transaction &tx) {
        int64_t maxValue = 0;
        for(auto input : tx.inputs()) {
            maxValue = std::max(input.getValue(), maxValue);
        }
        return maxValue;
    };
    
    auto combine = [](int64_t &a, int64_t &b) -> int64_t & { a = std::max(a,b); return a; };
    
    return chain.mapReduce<int64_t>(extract, combine);
}

int64_t calculateMaxFeeSingleThreaded(BlockRange &chain) {
    int64_t curMax = 0;
    for (auto block : chain) {
        RANGES_FOR(auto tx, block) {
            curMax = std::max(curMax, fee(tx));
        }
    }
    return curMax;
}

int64_t calculateMaxFeeMultithreaded(BlockRange &chain) {
    auto extract = [](const Transaction &tx) {
        return fee(tx);
    };
    
    auto combine = [](int64_t &a, int64_t &b) -> int64_t & { a = std::max(a,b); return a; };
    
    return chain.mapReduce<int64_t>(extract, combine);
}

int64_t calculateMaxFeeRandom(Blockchain &chain, const std::vector<uint32_t> &indexes) {
    int64_t maxValue = 0;
    for (auto index : indexes) {
        auto tx = Transaction(index, chain.getAccess());
        maxValue = std::max(maxValue, fee(tx));
    }
    return maxValue;
}

uint32_t calculateNonzeroLocktimeSingleThreaded(BlockRange &chain) {
    uint32_t count = 0;
    for (auto block : chain) {
        RANGES_FOR(auto tx, block) {
            count += tx.locktime() > 0;
        }
    }
    return count;
}

uint32_t calculateNonzeroLocktimeMultithreaded(BlockRange &chain) {
    auto extract = [](const Transaction &tx) {
        return static_cast<uint32_t>(tx.locktime() > 0);
    };
    
    auto combine = [](uint32_t &a, uint32_t &b) -> uint32_t & { a += b; return a; };
    
    return chain.mapReduce<uint32_t>(extract, combine);
}

uint32_t calculateNonzeroLocktimeRandom(Blockchain &chain, const std::vector<uint32_t> &indexes) {
    uint32_t nonzeroCount = 0;
    for (auto index : indexes) {
        auto tx = Transaction(index, chain.getAccess());
        nonzeroCount += static_cast<uint32_t>(tx.locktime() > 0);
    }
    return nonzeroCount;
}


template <typename Func, typename... Args>
auto timeFunc(std::string name, Func func, Args&& ...args) -> decltype(func(args...)) {
    auto begin = std::chrono::steady_clock::now();
    auto ret = func(std::forward<Args>(args)...);
    auto endTime = std::chrono::steady_clock::now();
    
    double timeSecs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - begin).count() / 1000000.0;
    std::cout << "Time in secs for " << name << ": " << timeSecs << std::endl;
    
    return ret;
}
