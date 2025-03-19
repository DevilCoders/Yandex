#pragma once

#include <kernel/lemmer/core/language.h>

namespace NIndexerCore {
namespace NIndexerCorePrivate {

    //! inteded for testing purposes only
    class ITokenProcessor {
    public:
        virtual ~ITokenProcessor()
        { }

        virtual void Lemmatize(const TWideToken& token, TWLemmaArray& lemmas, const TLangMask& langMask, const ELanguage* langs, const NLemmer::TAnalyzeWordOpt& options) = 0;
    };

} // NIndexerCorePrivate
} // NIndexerCore
