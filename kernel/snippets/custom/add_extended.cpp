#include "add_extended.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/trash_classifier/trash_classifier.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>

#include <library/cpp/containers/dense_hash/dense_hash.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/hash.h>

namespace NSnippets {

    namespace {
        const TUtf16String ELLIPSIS = u"...";
        const TUtf16String FOUR_DOTS = u"....";

        struct TWordHashesCollector : public ITokenHandler {
            TDenseHashSet<size_t> TokenHashes;

            void OnToken(const TWideToken& token, size_t /*origleng*/, NLP_TYPE type) override {
                if (type == NLP_WORD) {
                    TUtf16String word(token.Token, token.Leng);
                    TokenHashes.Insert(ComputeHash(word));
                }
            }
        };
    }

    static TDenseHashSet<size_t> ExtractUniqueWordHashes(const TUtf16String& text) {
        TWordHashesCollector collector;
        TNlpTokenizer tokenizer(collector);
        tokenizer.Tokenize(text);
        return collector.TokenHashes;
    }

    static size_t GetIntersectionSize(const TDenseHashSet<size_t>& first, const TDenseHashSet<size_t>& second) {
        size_t intersectionSize = 0;
        for (const size_t wordHash : first) {
            if (second.Has(wordHash)) {
                ++intersectionSize;
            }
        }
        return intersectionSize;
    }

    static bool HaveTooManyCommonWords(const TUtf16String& natural, const TUtf16String& extension) {
        const TDenseHashSet<size_t>& wordsFromNatural = ExtractUniqueWordHashes(natural);
        const TDenseHashSet<size_t>& wordsFromExtension = ExtractUniqueWordHashes(extension);
        size_t intersectionSize = GetIntersectionSize(wordsFromNatural, wordsFromExtension);
        return 2 * intersectionSize > std::min(wordsFromNatural.Size(), wordsFromExtension.Size());
    }

    static bool CheckLanguage(const TUtf16String& text, ELanguage lang) {
        return MeasureFractionOfLang(text, lang) > 0.5;
    }

    static void AddDots(TUtf16String& passage) {
        passage += ELLIPSIS;
        while (passage.EndsWith(FOUR_DOTS)) {
            passage.pop_back();
        }
        passage.append(' ');
    }

    bool CheckAndAddExtension(TUtf16String& extension, size_t maxExtLen, const TReplaceContext& ctx,
        TMultiCutResult& extDesc, ELanguage lang)
    {
        if (!ctx.Cfg.IsTouchReport() || !extension || !CheckLanguage(extension, lang) ||
            NTrashClassifier::IsTrash(ctx, extension) ||
            TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(extension).GetWeight() < 0.1 ||
            HaveTooManyCommonWords(extDesc.Short, extension))
        {
            return false;
        }

        SmartCut(extension, ctx.IH, maxExtLen, TSmartCutOptions(ctx.Cfg));
        AddDots(extDesc.Long);
        extDesc.Long += extension;
        extDesc.CharCountDifference = extDesc.Long.size() - extDesc.Short.size();
        return true;
    }

}
