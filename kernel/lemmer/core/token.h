#pragma once

#include <util/generic/vector.h>
#include <library/cpp/token/token_structure.h>
#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/alpha/alphaux.h>

namespace NLemmer {
    class TClassifiedMultiToken;
}
namespace NLemmerTokenNormalize {
    struct TTokenInfo {
        TTokenInfo()
            : Language()
            , Case(0)
        {
        }
        TTokenInfo(TLangMask lang, TCharCategory cs)
            : Language(lang)
            , Case(cs)
        {
        }

        TLangMask Language;
        TCharCategory Case;

        bool operator==(const TTokenInfo& ot) const {
            return Language == ot.Language && Case == ot.Case;
        }
        bool operator!=(const TTokenInfo& ot) const {
            return !(*this == ot);
        }
    };

    class TTokenBuffer {
    private:
        static const size_t StaticBufferSize = 256;
        NLemmerAux::TBuffer<StaticBufferSize> Buffer;
        TWideToken MToken;

    public:
        TTokenBuffer() {
            MToken.Token = Buffer(0);
        }
        const TWideToken& operator()() const {
            return MToken;
        }
        void Reserve(size_t size) {
            Buffer(size);
        }
        template <class TCopier>
        size_t AddChars(const wchar16* str, size_t len, const TCopier& copier);
        void AddSubToken(size_t offset, size_t len);
    };
    void Normalize(const TWideToken& orig, TTokenBuffer& res1, TTokenBuffer& res2, TVector<TTokenInfo>& info, bool primary);
    TTokenInfo Accumulate(const TVector<TTokenInfo>& info);

    template <class TI, class F>
    TCharCategory MergeCaseFlags(const TI& first, const TI& last, const F& getCaseFlags) {
        if (first == last)
            return CC_EMPTY;

        TCharCategory orCategories = (CC_DIFFERENT_ALPHABET | CC_HAS_DIACRITIC);
        TCharCategory res = getCaseFlags(first);
        TI ptr(first);
        ++ptr;
        for (; ptr != last; ++ptr) {
            TCharCategory orCategoriesSave = res & orCategories;
            auto caseFlags = getCaseFlags(ptr);
            if (caseFlags & CC_LOWERCASE)
                res &= (caseFlags | CC_TITLECASE); // lowercase continuations don't spoil title case
            else
                res &= caseFlags;
            res |= (caseFlags & orCategories) | orCategoriesSave;
        }

        //-- Single uppercase characters are treated as title case
        //        if ((res & CC_UPPERCASE) && (res & CC_TITLECASE))
        //             res &= ~CC_UPPERCASE;

        if (!(res & (CC_TITLECASE | CC_UPPERCASE | CC_LOWERCASE)))
            res |= CC_MIXEDCASE;

        return res;
    }
}

namespace NLemmer {
    class TClassifiedMultiToken {
    private:
        TWideToken Original_;

        /// Buffers for generic (1) and turkish (2) lowercase results.
        /* @note These buffers were originally used for performance
          * improvements.  However after commit r1361085 this is most probably
          * not necessary, as all the conversions are done as a single
          * transform.
          */
        NLemmerTokenNormalize::TTokenBuffer MToken1;
        NLemmerTokenNormalize::TTokenBuffer MToken2;

        TVector<NLemmerTokenNormalize::TTokenInfo> Info;
        NLemmerTokenNormalize::TTokenInfo Cumulative;

    public:
        explicit TClassifiedMultiToken(const TWideToken& tok = TWideToken(), bool primary=true)
            : Original_(tok)
        {
            Normalize(primary);
        }

        TClassifiedMultiToken(const wchar16* s, size_t len, bool primary=true)
            : Original_(s, len)
        {
            Normalize(primary);
        }

        const TWideToken& Original() const {
            return Original_;
        }

        const TWideToken& Multitoken(ELanguage id = LANG_UNK) const {
            return ChooseMT(id);
        }
        const wchar16* Str(ELanguage id = LANG_UNK) const {
            return ChooseMT(id).Token;
        }
        size_t Length(ELanguage id = LANG_UNK) const {
            return ChooseMT(id).Leng;
        }

        const TCharSpan& Token(size_t i, ELanguage id = LANG_UNK) const {
            Y_ASSERT(i < NumTokens());
            return ChooseMT(id).SubTokens[i];
        }

        size_t NumTokens() const {
            Y_ASSERT(Info.size() == MToken1().SubTokens.size());
            Y_ASSERT(Info.size() == MToken2().SubTokens.size());
            Y_ASSERT(Info.size() == Original_.SubTokens.size());
            return Info.size();
        }

        TLangMask TokenAlphaLanguages(size_t i) const {
            Y_ASSERT(i < NumTokens());
            return Info[i].Language;
        }
        TCharCategory TokenAlphaCase(size_t i) const {
            Y_ASSERT(i < NumTokens());
            return Info[i].Case;
        }

        TLangMask CumulativeAlphaLanguage() const {
            return Cumulative.Language;
        }
        TCharCategory CumulativeAlphaCase() const {
            return Cumulative.Case;
        }

    private:
        void Normalize(bool primary) {
            NLemmerTokenNormalize::Normalize(Original_, MToken1, MToken2, Info, primary);
            Cumulative = NLemmerTokenNormalize::Accumulate(Info);
        }
        const TWideToken& ChooseMT(ELanguage id) const {
            if (id == LANG_TUR)
                return MToken2();
            return MToken1();
        }
    };
}
