#include "decapital.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/data/abbrev_words.h>

#include <kernel/snippets/strhl/zonedstring.h>

#include <kernel/lemmer/alpha/abc.h>

#include <util/generic/vector.h>
#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/hash_set.h>

namespace NSnippets {

class TDecapitalizer::TImpl {
private:
    struct TQuote {
        size_t Beg;
        size_t End;
        bool OnlyUpper; // = true means that this quote should be lowercase BUT first letter is supposed to be upper
    };
    typedef TVector< std::pair<size_t, size_t> > TWords; // It could contain duplicates. Use GetFairWordsSize() to know its size
    struct TDecapSent {
        size_t SentId;
        bool NeedDecap;
        TWords Words;
    };

public:
    TVector< TDecapSent > Sents;
    size_t CurrentSentId;
    TVector<size_t> AlphasToUpper;
    const TUtf16String &InitSent;
    TUtf16String Sent;
    bool NeedFirstUpper;
    TVector<TQuote> Quotes;
    const NLemmer::TAlphabetWordNormalizer* WordNormalizer;

private:
    inline int GetFairWordsSize(const TDecapSent& sent) const {
        THashSet< std::pair<size_t, size_t> > words;
        for (size_t k = 0; k < sent.Words.size(); ++k) {
            words.insert(sent.Words[k]);
        }
        return (int)words.size();
    }

    inline bool IsRomanianNumber(const TWtringBuf &w) const {
        for (size_t i = 0; i < w.size(); ++i) {
            if (!IsRomanDigit(*(w.data() + i))) {
                return false;
            }
        }
        return true;
    }

    bool IsAbbr(const TWtringBuf &w) const;

    inline void ToLower(const size_t st, const size_t len) {
        for (size_t offset = 0; offset < len; ++offset) {
            if (st + offset >= Sent.Size()) {
                break;
            }
            Sent[st + offset] = WordNormalizer->ToLower(Sent[st + offset]);
        }
    }

    // actually this function is not To Upper, but To "AsItWas", but in this case it's the same
    inline void ToUpper(const size_t st, const size_t len) {
        for (size_t offset = 0; offset < len; ++offset) {
            if (st + offset >= Sent.Size() || st + offset >= InitSent.Size()) {
                break;
            }
            Sent[st + offset] = InitSent[st + offset];
        }
    }

    inline bool WasChanged(size_t pos) const {
        Y_ENSURE(pos < Sent.Size() && pos < InitSent.Size());
        return Sent[pos] != InitSent[pos];
    }

    inline void Init() {
        NeedFirstUpper = true;
        Quotes.resize(0);
    }

    void Prepair() {
        if (Sents.empty()) {
            return;
        }
        int len = GetFairWordsSize(*Sents.begin());
        size_t firstId = 0;
        size_t lastSentId = Sents.begin()->SentId;
        for (size_t i = 1; i < Sents.size(); ++i) {
            if (lastSentId + 1 == Sents[i].SentId) {    // sentences go in a row
                len += GetFairWordsSize(Sents[i]);
                ++lastSentId;
            } else {
                if (len > 2) {
                    for (size_t j = firstId; j < i; ++j) {
                        Sents[j].NeedDecap = true;
                    }
                }
                len = GetFairWordsSize(Sents[i]);
                firstId = i;
                lastSentId = Sents[i].SentId;
            }
        }
        if (len > 2) {
            for (size_t j = firstId; j < Sents.size(); ++j) {
                Sents[j].NeedDecap = true;
            }
        }
    }

    // In this case word is sequence of alphas, digits & dashes
    bool NextWord(size_t &off, size_t end, TWtringBuf &buf, bool &isAlphaDigitMix);

    bool NeedDecapital(const wchar16 *pc, const size_t len) const;

    void MarkQuotes(const wchar16 *pc, const size_t len);

    void MakeQuotes();

    void SimpleLower(size_t beg, const size_t end);

public:
    TImpl(const TUtf16String& sent, ELanguage lang)
        : CurrentSentId(0)
        , InitSent(sent)
        , Sent(sent)
        , NeedFirstUpper(true)
        , WordNormalizer(NLemmer::GetAlphaRules(lang))
    {
    }
    void DecapitalSentence(const wchar16 *pc, const size_t len);
    void DecapitalSingleWord(const wchar16 *pc, const size_t len, bool needFirstUpper);
    void RevertFioChanges(const wchar16 *pc, const size_t len);
    void Complete(TZonedString &z);
};
TDecapitalizer::TDecapitalizer(const TUtf16String& sent, ELanguage lang)
  : Impl(new TImpl(sent, lang))
{
}
TDecapitalizer::~TDecapitalizer() {
}
void TDecapitalizer::DecapitalSentence(const wchar16* pc, const size_t len) {
    Impl->DecapitalSentence(pc, len);
}
void TDecapitalizer::DecapitalSingleWord(const wchar16* pc, const size_t len, bool needFirstUpper) {
    Impl->DecapitalSingleWord(pc, len, needFirstUpper);
}
void TDecapitalizer::RevertFioChanges(const wchar16* pc, const size_t len) {
    Impl->RevertFioChanges(pc, len);
}
void TDecapitalizer::Complete(TZonedString& z) {
    Impl->Complete(z);
}
TUtf16String TDecapitalizer::GetSent() const {
    return Impl->Sent;
}

inline bool TDecapitalizer::TImpl::IsAbbr(const TWtringBuf &w) const {
        return TAbbrevContainer::GetDefault().Contains(WideToUTF8(w.data(), w.size()));
}

bool TDecapitalizer::TImpl::NextWord(size_t &off, size_t end, TWtringBuf &buf, bool &hasDigits) {
    hasDigits = false;
    bool inWord = false;
    size_t st = off;
    for (; off < end; ++off) {
        if (IsAlnum(*(InitSent.data() + off)) || IsDash(*(InitSent.data() + off))) {
            if (!inWord) {
                st = off;
            }
            if (IsDigit(*(InitSent.data() + off))) {
                hasDigits = true;
            }
            inWord = true;
        } else {
            if (inWord) {
                break;
            }
            if (!IsBlank(*(InitSent.data() + off))) {
                NeedFirstUpper = false;
            }
        }
    }
    if (inWord) {
        if (!IsAlpha(*(InitSent.data() + st))) {    // first symbol could be a digit
            NeedFirstUpper = false;
        }
        buf = TWtringBuf(InitSent.data() + st, InitSent.data() + off);
        return true;
    }
    return false;
}

inline bool TDecapitalizer::TImpl::NeedDecapital(const wchar16 *pc, const size_t len) const {
    int alpha = 0;
    int upperAlpha = 0;
    for (size_t i = 0; i < len; ++i) {
        if (IsAlpha(*(pc + i))) {
            ++alpha;
            if (IsUpper(*(pc + i))) {
                ++upperAlpha;
            }
        }
    }
    return alpha > 0 && upperAlpha * 100 > alpha * 95;
}

#define SHIFT(i) (ULL(1) << (i))

static bool IsLeftQuote(wchar32 ch) {
    return NUnicode::CharHasType(ch, SHIFT(Po_QUOTE) | SHIFT(Ps_QUOTE) | SHIFT(Pi_QUOTE) |
        SHIFT(Po_SINGLE_QUOTE) | SHIFT(Ps_SINGLE_QUOTE) | SHIFT(Pi_SINGLE_QUOTE));
}

static bool IsRightQuote(wchar32 ch) {
    return NUnicode::CharHasType(ch, SHIFT(Po_QUOTE) | SHIFT(Pe_QUOTE) | SHIFT(Pf_QUOTE) |
        SHIFT(Po_SINGLE_QUOTE) | SHIFT(Pe_SINGLE_QUOTE) | SHIFT(Pf_SINGLE_QUOTE));
}

void TDecapitalizer::TImpl::MarkQuotes(const wchar16 *pc, const size_t len) {
    bool inQuote = false;;
    size_t pos = 0;
    for (; pos < len; ++pos) {
        if (IsLeftQuote(*(pc + pos))) {
            inQuote = true;
            ++pos;
        }
        if (!inQuote) {
            continue;
        }

        size_t begQuote = pos;
        size_t endQuote = pos;
        bool onlyUpper = true;
        bool hasAlpha = false;
        for (; pos < len; ++pos) {
            if (IsAlpha(*(pc + pos))) {
                hasAlpha = true;
                if (!IsUpper(*(pc + pos))) {
                    onlyUpper = false;
                }
                continue;
            }
            if (IsRightQuote(*(pc + pos))) {
                endQuote = pos;
                break;
            }
            if (endQuote != begQuote) {
                break;
            }
        }
        if (pos >= len) {
            break;
        }

        ++pos;
        if (hasAlpha) {
            Quotes.emplace_back();
            Quotes.back().Beg = pc - InitSent.data() + begQuote;
            Quotes.back().End = pc - InitSent.data() + endQuote;
            Quotes.back().OnlyUpper = onlyUpper;
        }

        inQuote = false;
    }
}

void TDecapitalizer::TImpl::SimpleLower(size_t beg, const size_t end) {
    TWtringBuf buf;
    bool firstWord = true;
    bool hasDigits = false;
    while (NextWord(beg, end, buf, hasDigits)) {
        if (hasDigits || IsRomanianNumber(buf) || IsAbbr(buf)) {
            continue;
        } else {
            size_t offset = buf.data() - InitSent.data();
            if (buf.size() > 0) {
                Sents.back().Words.push_back(std::pair<size_t, size_t>(offset, buf.size()));
                if (firstWord && NeedFirstUpper) {
                    AlphasToUpper.push_back(offset);
                }
            }
        }
        firstWord = false;
    }
}

void TDecapitalizer::TImpl::MakeQuotes() {
    for (size_t i = 0; i < Quotes.size(); ++i) {
        if (!Quotes[i].OnlyUpper) {
            continue;
        }
        NeedFirstUpper = true;
        SimpleLower(Quotes[i].Beg, Quotes[i].End);
    }
}

void TDecapitalizer::TImpl::DecapitalSentence(const wchar16 *pc, const size_t len) {
    Init();
    ++CurrentSentId;
    if (!NeedDecapital(pc, len)) {
        return;
    }
    Sents.emplace_back();
    Sents.back().SentId = CurrentSentId;
    Sents.back().NeedDecap = false;

    size_t beg = pc - InitSent.data();
    size_t end = beg +len;
    SimpleLower(beg, end);
    MarkQuotes(pc, len);
    MakeQuotes();
}

void TDecapitalizer::TImpl::DecapitalSingleWord(const wchar16 *pc, const size_t len, bool needFirstUpper) {
    if (!NeedDecapital(pc, len)) {
        return;
    }
    Sents.emplace_back();
    Sents.back().SentId = CurrentSentId++;
    Sents.back().NeedDecap = false;

    const TWtringBuf buf(pc, len);
    if (len < 4 || IsRomanianNumber(buf) || IsAbbr(buf)) {
        return;
    }

    size_t offset = buf.data() - InitSent.data();
    Sents.back().Words.push_back(std::pair<size_t, size_t>(offset, buf.size()));
    if (needFirstUpper) {
        AlphasToUpper.push_back(offset);
    }
}

void TDecapitalizer::TImpl::RevertFioChanges(const wchar16 *pc, const size_t len) {
    TWtringBuf buf;
    bool hasDigits = false;
    size_t beg = pc - InitSent.data();
    size_t end = beg +len;
    while (NextWord(beg, end, buf, hasDigits)) {
        size_t pos = buf.data() - InitSent.data();
        AlphasToUpper.push_back(pos);
    }
}

void TDecapitalizer::TImpl::Complete(TZonedString &z) {
    Prepair();
    for (size_t i = 0; i < Sents.size(); ++i) {
        if (!Sents[i].NeedDecap) {
            continue;
        }
        for (size_t j = 0; j < Sents[i].Words.size(); ++j) {
            ToLower(Sents[i].Words[j].first, Sents[i].Words[j].second);
        }
    }
    for (size_t i = 0; i < AlphasToUpper.size(); ++i) {
        ToUpper(AlphasToUpper[i], 1);
    }

    // All this code is true only because of decapitalization doesn't change length of the String
    const wchar16 *initBegin = InitSent.data();
    z.String = Sent;
    for (TZonedString::TZones::iterator it = z.Zones.begin(); it != z.Zones.end(); ++it) {
        for (size_t i = 0; i < it->second.Spans.size(); ++i) {
            size_t offset = ~(it->second.Spans[i]) - initBegin;
            it->second.Spans[i].Span = TWtringBuf(z.String.data() + offset, +it->second.Spans[i]);
        }
    }
}

void DecapitalSentence(TUtf16String &w, ELanguage lang) {
    TDecapitalizer decapitalizer(w, lang);
    decapitalizer.DecapitalSentence(w.data(), w.size());
    TZonedString tmpZ;
    decapitalizer.Complete(tmpZ);
    w = decapitalizer.GetSent();
}

}
