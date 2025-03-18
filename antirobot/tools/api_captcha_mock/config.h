#pragma once

#include <util/generic/hash.h>

class TStrategyConfig {
public:
    TString GenerateStrategy;
    TString CheckStrategy;

    static TStrategyConfig& Get() {
        static TStrategyConfig cfg;
        return cfg;
    }
};

#define STRATEGY_CONFIG TStrategyConfig::Get()
