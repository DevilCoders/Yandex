#pragma once

#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>

#include "common.h"
#include "lexical_decomposition_algo.h"

namespace NLexicalDecomposition {
    class TTokenLexicalSplitter : TNonCopyable {
    public:
        TTokenLexicalSplitter(const TBlob& blob, bool = false, bool visualize = false, ui32 options = DO_DEFAULT);

        /// Returns true if `token' provides result better than previous tokens
        bool ProcessToken(const TUtf16String& token, ELanguage language = LANG_UNK);

        const TUtf16String& operator[](size_t index) const;
        size_t GetResultSize() const {
            return ResultWords.size();
        }
        ELanguage GetResultLanguage() const {
            return ResultLanguage;
        }
        size_t GetBestProcessedTokenId() const {
            return BestProcessedTokenId;
        }

        const TString ProcessUntranslitOutput(const TString& line);

    private:
        const TBlob Blob;
        TNamedChunkedDataReader Reader;

        bool Visualize;
        ui32 Options;

        TDecompositionResultDescr ResultDescr;
        TVector<TUtf16String> ResultWords;
        ELanguage ResultLanguage;

        size_t BestProcessedTokenId;
        size_t ProcessedTokensAmount;
    };

}
