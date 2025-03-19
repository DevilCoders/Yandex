#pragma once
#include <library/cpp/yconf/conf.h>
#include <util/generic/hash_set.h>
#include <util/stream/str.h>

namespace NCommonProxy {

    struct TLinkConfig {
        TString From;
        TString To;
        ui64 RpsLimit = 0;
        bool Enabled = true;
        THashSet<TString> IgnoredSignals;

        void Init(const TYandexConfig::Section& section);
        void ToString(TStringOutput& so) const;
        bool IsIgnoredSignal(const TString& signal) const;
    };

}
