#pragma once

#include <util/generic/hash.h>

class TStrategyConfig {
public:
    TString Strategy;
    TString ButtonStrategy;

    static TStrategyConfig& Get() {
        static TStrategyConfig cfg;
        return cfg;
    }
};

#define STRATEGY_CONFIG TStrategyConfig::Get()
