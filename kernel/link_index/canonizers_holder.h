#pragma once

#include <library/cpp/deprecated/calc_module/simple_module.h>

#include <kernel/erfcreator/canonizers.h> // for TCanonizers

// Canonizers holder
// Use mirrors.hash(.res or .trie) files
class TCanonizersHolder : public TSimpleModule {
private:
    const TErfCreateConfig ErfCreateConfig;
    TCanonizers Canonizers;

public:
    TCanonizersHolder(const TErfCreateConfig& erfCreateConfig)
        : TSimpleModule("TCanonizersHolder")
        , ErfCreateConfig(erfCreateConfig)
    {
        Bind(this).To<const TCanonizers*>(&Canonizers, "canonizers_output");
        Bind(this).To<&TCanonizersHolder::Init>("init");
    }

private:
    void Init() {
        Canonizers.InitCanonizers(nullptr, GetOwnerResolver(ErfCreateConfig));
    }
};
