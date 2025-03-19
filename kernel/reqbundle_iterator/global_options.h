#pragma once

#include <kernel/lingboost/constants.h>

#include <library/cpp/langmask/langmask.h> // LemmasInIndex

#include <util/generic/maybe.h>

namespace NReqBundleIterator {
    struct TGlobalOptions { // must be the same for all iterators
        TLangMask LemmatizedLanguages = NLanguageMasks::LemmasInIndex();
        TMaybe<NLingBoost::EExpansionType> RequestExpansionType;

        TGlobalOptions() = default;
        explicit TGlobalOptions(const TLangMask& lemmatizedLanguages)
            : LemmatizedLanguages(lemmatizedLanguages)
        {}
    };
}
