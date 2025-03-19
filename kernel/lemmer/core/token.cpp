#include <kernel/lemmer/alpha/abc.h>

#include "token.h"
#include "language.h"
#include "lemmeraux.h"

namespace {
    struct TMemCopy {
        size_t Copy(wchar16* to, const wchar16* from, size_t len) const {
            memcpy(to, from, len * sizeof(wchar16));
            return len;
        };
    };

    struct TLowerCaseCopy {
        const NLemmer::TAlphabetWordNormalizer* Awn;
        TLowerCaseCopy(ELanguage lang)
            : Awn(NLemmer::GetAlphaRules(lang))
        {
        }
        size_t Copy(wchar16* to, const wchar16* from, size_t len) const {
            return Awn->ToLower(from, len, to, len).Length;
        };
    };
}

namespace {
    class TStringBuffer : TNonCopyable {
    private:
        static const size_t InitialSize = 1024;
        NLemmerAux::TBuffer<InitialSize> Buffer;
        size_t Len;

    public:
        TStringBuffer()
            : Len(0)
        {
        }
        void push_back(wchar16 c) {
            Buffer(Len + 1)[Len] = c;
            ++Len;
        }
        size_t Length() const {
            return Len;
        }
        const wchar16* GetStr() const {
            return Buffer.GetPtr();
        }
    };

    class TDecomposer : TNonCopyable {
        TStringBuffer String;

    public:
        TDecomposer(const wchar16* str, size_t len) {
            NLemmerAux::Decompose(str, len, String);
        }
        const wchar16* GetStr() const {
            return String.GetStr();
        }
        size_t GetLength() const {
            return String.Length();
        }
    };
}

namespace NLemmerTokenNormalize {
    template <class TCopier>
    size_t TTokenBuffer::AddChars(const wchar16* str, size_t len, const TCopier& copier) {
        wchar16* buf = Buffer(MToken.Leng + len);
        MToken.Token = buf;
        len = copier.Copy(buf + MToken.Leng, str, len);
        MToken.Leng += len;
        return len;
    }

    void TTokenBuffer::AddSubToken(size_t offset, size_t len) {
        MToken.SubTokens.push_back(offset, len);
    }

    template <class TCopier>
    std::pair<size_t, size_t> AddCharsInt(const wchar16* str, size_t len,
                                          const TCopier& copier1, const TCopier& copier2,
                                          TTokenBuffer& tb1, TTokenBuffer& tb2) {
        std::pair<size_t, size_t> ret(0, 0);
        ret.first = tb1.AddChars(str, len, copier1);
        ret.second = tb2.AddChars(str, len, copier2);
        return ret;
    }

    std::pair<size_t, size_t> AddStrings(const TDecomposer& dec, TTokenBuffer& tb1, TTokenBuffer& tb2) {
        return AddCharsInt(dec.GetStr(), dec.GetLength(), TMemCopy(), TMemCopy(), tb1, tb2);
    }

    std::pair<size_t, size_t> AddSubTokens(const TDecomposer& dec, TTokenBuffer& tb1, TTokenBuffer& tb2) {
        size_t l01 = tb1().Leng;
        size_t l02 = tb2().Leng;
        std::pair<size_t, size_t> ret = AddCharsInt(dec.GetStr(), dec.GetLength(), TLowerCaseCopy(LANG_UNK), TLowerCaseCopy(LANG_TUR), tb1, tb2);
        tb1.AddSubToken(l01, ret.first);
        tb2.AddSubToken(l02, ret.second);
        return ret;
    }

#if defined __clang__ && __clang_major__ == 3 && __clang_minor__ == 9
    namespace {
        __attribute__((noinline)) void Reserve(TVector<TTokenInfo>& info, size_t size) {
            info.reserve(size);
        }
    }
#endif

    void Normalize(const TWideToken& multitoken, TTokenBuffer& res1, TTokenBuffer& res2,
                   TVector<TTokenInfo>& info, bool primary) {
        const wchar16* word = multitoken.Token;
        const TTokenStructure& tokens = multitoken.SubTokens;

        info.clear();
#if defined __clang__ && __clang_major__ == 3 && __clang_minor__ == 9
        Reserve(info, tokens.size());
#else
        info.reserve(tokens.size());
#endif
        res1.Reserve(multitoken.Leng);
        res2.Reserve(multitoken.Leng);

        for (size_t i = 0; i < tokens.size(); i++) {
            const TCharSpan& s = tokens[i];

            // Copy the separator
            size_t lastend = i ? tokens[i - 1].EndPos() : 0;
            size_t sepsize = s.Pos - lastend;
            if (sepsize) {
                TDecomposer decomposer(word + lastend, sepsize);
                AddStrings(decomposer, res1, res2);
            }

            // Copy the token itself
            assert(s.Len > 0);

            TDecomposer dec(word + s.Pos, s.Len);

            TCharCategory cs = NLemmer::ClassifyCase(word + s.Pos, s.Len);
            TLangMask lang = NLemmer::ClassifyLanguage(dec.GetStr(), dec.GetLength(), false, primary);

            AddSubTokens(dec, res1, res2);
            info.push_back(TTokenInfo(lang, cs));
        }

        // Copy the very last suffix
        if (tokens.size()) {
            size_t lastend = tokens.back().EndPos();
            size_t sufsize = multitoken.Leng - lastend;
            if (sufsize) {
                TDecomposer decomposer(word + lastend, sufsize);
                AddStrings(decomposer, res1, res2);
            }
        }
    }

    template <class TValue>
    struct TCaseGetter {
        template <class TIt>
        TValue* operator()(const TIt& it) const {
            return &(it->Case);
        }
    };

    TTokenInfo Accumulate(const TVector<TTokenInfo>& info) {
        TLangMask lang;
        for (size_t i = 0; i < info.size(); ++i)
            lang |= info[i].Language;

        TCharCategory cs = MergeCaseFlags(
            info.begin(),
            info.end(),
            [](TVector<TTokenInfo>::const_iterator token) -> TCharCategory { return token->Case; });

        return TTokenInfo(
            lang,
            cs);
    }
}
