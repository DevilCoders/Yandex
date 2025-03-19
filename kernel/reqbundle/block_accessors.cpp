#include "block_accessors.h"

#include "size_limits.h"

namespace {
    using namespace NReqBundle;

    inline i64 CompareInternal(TConstWordAcc x, TConstWordAcc y) {
        if (x.IsValid() && y.IsValid()) {
            const int textCmp = TString::compare(x.GetText(), y.GetText());
            if (!!textCmp) {
                return textCmp;
            }
            if (x.GetLangMask() < y.GetLangMask()) {
                return -1;
            } else if (y.GetLangMask() < x.GetLangMask()) {
                return 1;
            }

            i64 res = 0;

            if (res = static_cast<i64>(x.GetNlpType()) - static_cast<i64>(y.GetNlpType())) {
                return res;
            } else if (res = static_cast<i64>(x.GetCaseFlags()) - static_cast<i64>(y.GetCaseFlags())) {
                return res;
            } else {
                return static_cast<i64>(x.IsStopWord()) - static_cast<i64>(y.IsStopWord());
            }
        } else {
            return static_cast<int>(x.IsValid()) - static_cast<int>(y.IsValid());
        }
    }

    inline i64 CompareInternal(TConstBlockAcc x, TConstBlockAcc y) {
        if (x.IsValid() && y.IsValid()) {
            if (x.GetNumWords() == y.GetNumWords()) {
                for (size_t i : xrange(x.GetNumWords())) {
                    i64 wordCmp = CompareInternal(x.GetWord(i), y.GetWord(i));
                    if (!!wordCmp) {
                        return wordCmp;
                    }
                }
            } else {
                return static_cast<int>(x.GetNumWords()) - static_cast<int>(y.GetNumWords());
            }
        } else {
            return static_cast<int>(x.IsValid()) - static_cast<int>(y.IsValid());
        }

        return 0;
    }
} // namespace

namespace NReqBundle {
    int Compare(TConstWordAcc x, TConstWordAcc y) {
        i64 res = CompareInternal(x, y);
        return res < 0 ? -1 : (res > 0 ? 1 : 0);
    }

    int Compare(TConstBlockAcc x, TConstBlockAcc y) {
        i64 res = CompareInternal(x, y);
        return res < 0 ? -1 : (res > 0 ? 1 : 0);
    }

    namespace NDetail {
        EValidType IsValidLemma(TConstLemmaAcc lemma) {
            const bool ok =  lemma.GetNumForms() <= TSizeLimits::MaxNumForms;
            return (ok ? VT_TRUE : VT_FALSE);
        }
        EValidType IsValidWord(TConstWordAcc word) {
            if (word.GetNumLemmas() > TSizeLimits::MaxNumLemmas) {
                return VT_FALSE;
            }
            const bool allAttributeLemmas = word.GetNumLemmas() > 0 && !AnyOf(word.GetLemmas(), [](TConstLemmaAcc lemma) { return !lemma.IsAttribute(); });
            if (word.IsAnyWord() || allAttributeLemmas) {
                if (word.GetNlpType() != NLP_END) {
                    return VT_FALSE;
                }
            } else if (word.GetNlpType() == NLP_END) {
                return VT_FALSE;
            }
            return VT_TRUE;
        }
        EValidType IsValidBlock(TConstBlockAcc block) {
            bool ok = block.GetNumWords() >= 1
                    && block.GetNumWords() <= TSizeLimits::MaxNumWordsInBlock;

            if (block.GetType() != NDetail::EBlockType::ExactOrdered) {
                for (const auto& word : block.GetWords()) {
                    if (word.IsAnyWord()) {
                        ok = false;
                        break;
                    }
                }
            }
            for (size_t i = 0; i < block.GetNumWords() && ok; ++i) {
                EValidType wordValidType = IsValidWord(block.GetWord(i));
                Y_ASSERT(wordValidType != VT_UNKNOWN);
                ok &= (wordValidType == VT_TRUE);
            }
            return (ok ? VT_TRUE : VT_FALSE);
        }
    } // NDetail
} // NReqBundle
