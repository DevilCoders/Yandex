#pragma once
#include <kernel/common_server/library/async_impl/config.h>

class TStartrekClientConfig : public TTokenAuthRequestConfig {
    using TBase = TTokenAuthRequestConfig;

public:
    static TStartrekClientConfig ParseFromString(const TString& configStr) {
        return TBase::ParseFromString<TStartrekClientConfig>(configStr);
    }
};
