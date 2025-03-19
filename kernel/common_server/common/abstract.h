#pragma once
#include <util/generic/ptr.h>

class IUserProcessorCustomizer {
public:
    using TPtr = TAtomicSharedPtr<IUserProcessorCustomizer>;

    virtual ~IUserProcessorCustomizer() = default;

    virtual bool OverrideOption(const TString& /*key*/, TString& /*value*/) const {
        return false;
    }
};
