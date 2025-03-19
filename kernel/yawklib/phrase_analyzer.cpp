#include <util/generic/hash_set.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include "phrase_analyzer.h"
#include "wtrutil.h"
#include "declension.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TEmptyRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TEmptyRecognizer);
public:
    bool IsMatchImpl(const TUtf16String &, size_t, size_t, TUtf16String*) const override {
        return false;
    }
    bool AddAllMatches(TVector<TUtf16String>*) const override {
        return true;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsSubstring(const TUtf16String &string, size_t from, size_t to, const TUtf16String &substring)
{
    if (to - from != substring.size())
        return false;
    Y_ASSERT(to >= from);
    for (size_t n = 0, m = from; m < to; ++m, ++n)
        if (string[m] != substring[n])
            return false;
    return true;
}
struct TSingleWordRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TSingleWordRecognizer);
public:
    TUtf16String Phrase;
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        bool ret = IsSubstring(phrase, from, to, Phrase);
        if (ret && res)
            *res = Phrase;
        return ret;
    }
    bool AddAllMatches(TVector<TUtf16String> *res) const override {
        res->push_back(Phrase);
        return true;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TFiniteSetRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TFiniteSetRecognizer);
public:
    enum {
        SIZE_THRESHOLD = 400000
    };
    THashSet<TUtf16String> Phrases;
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        TUtf16String cur = phrase.substr(from, to - from);
        bool ret = Phrases.find(cur) != Phrases.end();
        if (ret) {
            if (res)
                *res = cur;
        }
        return ret;
    }
    bool AddAllMatches(TVector<TUtf16String> *matches) const override {
        matches->reserve(Phrases.size());
        for (THashSet<TUtf16String>::const_iterator it = Phrases.begin(); it != Phrases.end(); ++it)
            matches->push_back(*it);
        return true;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateRecognizer(const TVector<TUtf16String> &matches) {
    if (matches.empty())
        return new TEmptyRecognizer;
    else if (matches.size() == 1) {
        TSingleWordRecognizer *ret = new TSingleWordRecognizer;
        ret->Phrase = matches[0];
        return ret;
    } else {
        TFiniteSetRecognizer *ret = new TFiniteSetRecognizer;
        for (size_t n = 0; n < matches.size(); ++n)
            ret->Phrases.insert(matches[n]);
        return ret;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TFiniteTranslatorRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TFiniteTranslatorRecognizer);
public:
    THashMap<TUtf16String, TUtf16String> Translations;
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        TUtf16String cur = phrase.substr(from, to - from);
        THashMap<TUtf16String, TUtf16String>::const_iterator it = Translations.find(cur);
        if (it == Translations.end())
            return false;
        if (res)
            *res = it->second;
        return true;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateTranslator(const TVector<std::pair<TUtf16String, TUtf16String> > &translations)
{
    bool allTrivial = true;
    for (size_t n = 0; n < translations.size(); ++n) {
        if (translations[n].first != translations[n].second) {
            allTrivial = false;
            break;
        }
    }
    if (allTrivial) {
        TVector<TUtf16String> matches(translations.size());
        for (size_t n = 0; n < translations.size(); ++n)
            matches[n] = translations[n].first;
        return CreateRecognizer(matches);
    } else {
        TFiniteTranslatorRecognizer *ret = new TFiniteTranslatorRecognizer;
        for (size_t n = 0; n < translations.size(); ++n)
            ret->Translations[translations[n].first] = translations[n].second;
        return ret;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TUnionRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TUnionRecognizer);
public:
    THashSet<TUtf16String> Phrases;
    TVector<TPtr<IPhraseRecognizer> > Parts;

    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        if (!Phrases.empty()) {
            TUtf16String cur = phrase.substr(from, to - from);
            if (Phrases.find(cur) != Phrases.end()) {
                if (res)
                    *res = cur;
                return true;
            }
        }
        for (size_t n = 0; n < Parts.size(); ++n)
            if (Parts[n]->CollectMatch(phrase, from, to, res))
                return true;
        return false;
    }
};
IPhraseRecognizer* Union(const TVector<TPtr<IPhraseRecognizer> > &parts)
{
    TVector<TUtf16String> all;
    TVector<TPtr<IPhraseRecognizer> > complexParts;
    for (size_t n = 0; n < parts.size(); ++n) {
        if (!parts[n]->AddAllMatches(&all))
            complexParts.push_back(parts[n]);
        else if (all.size() > TFiniteSetRecognizer::SIZE_THRESHOLD && n < parts.size() - 1) {
            complexParts.push_back(CreateRecognizer(all));
            all.clear();
        }
    }
    if (complexParts.empty())
        return CreateRecognizer(all);
    else {
        TUnionRecognizer *res = new TUnionRecognizer;
        res->Parts.swap(complexParts);
        for (size_t n = 0; n < all.size(); ++n)
            res->Phrases.insert(all[n]);
        return res;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSubtractRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TSubtractRecognizer);
public:
    TPtr<IPhraseRecognizer> Recognizer;
    TPtr<IPhraseRecognizer> Stoplist;

    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        return Recognizer->CollectMatch(phrase, from, to, res) && !Stoplist->IsMatch(phrase, from, to);
    }
};
IPhraseRecognizer* Subtract(TPtr<IPhraseRecognizer> recognizer, TPtr<IPhraseRecognizer> stoplist)
{
    TVector<TUtf16String> all;
    if (recognizer->AddAllMatches(&all)) {
        TVector<TUtf16String> selected; selected.reserve(all.size());
        for (size_t n = 0; n < all.size(); ++n) {
            TUtf16String phrase = all[n];
            if (!stoplist->IsMatch(phrase, 0, phrase.size()))
                selected.push_back(phrase);
        }
        return CreateRecognizer(selected);
    }
    TSubtractRecognizer *ret = new TSubtractRecognizer;
    ret->Recognizer = recognizer;
    ret->Stoplist = stoplist;
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool HasInSequence(
    const TVector<std::pair<unsigned, TUtf16String> > &seq, unsigned what)
{
    for (size_t n = 0; n < seq.size(); ++n)
        if (seq[n].first == what)
            return true;
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSequenceRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TSequenceRecognizer);
public:
    TUtf16String Prefix;
    TUtf16String Suffix;
    TVector<TPtr<IPhraseRecognizer> > Parts;
    TVector<bool> Spaces;
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        size_t prefixEnd = from + (Prefix.size());
        if (prefixEnd > to || !IsSubstring(phrase, from, prefixEnd, Prefix))
            return false;
        size_t suffixStart = to - (Suffix.size());
        if (suffixStart < prefixEnd || !IsSubstring(phrase, suffixStart, to, Suffix))
            return false;
        if (Parts.size() == 1) {
            bool ret = Parts[0]->CollectMatch(phrase, prefixEnd, suffixStart, res);
            if (res && ret)
                *res = Prefix + *res + Suffix;
            return ret;
        }
        if (Parts.size() > 63)
            ythrow yexception() << "sequence recognizer of 64 parts or more currently unsupported";
        Y_ASSERT(suffixStart >= prefixEnd);
        if (!res) {
            TVector<ui64> sequence(suffixStart - prefixEnd + 1);
            sequence[0] = 1;
            for (size_t cur = 0; cur < sequence.size(); ++cur) {
                for (ui64 n = 0; n < Parts.size(); ++n) {
                    ui64 bit = 1 << n;
                    if ((sequence[cur] & bit) == 0)
                        continue;
                    IPhraseRecognizer *part = Parts[n];
                    size_t start = prefixEnd + cur;
                    if (n == Parts.size() - 1) {
                        if (part->IsMatch(phrase, start, suffixStart))
                            return true;
                    } else {
                        for (size_t end = start; end <= suffixStart; ++end) {
                            if (Spaces[n] && end > start && end < to && phrase[end] != ' ')
                                continue;
                            if (part->IsMatch(phrase, start, end)) {
                                size_t next = end - prefixEnd;
                                if (Spaces[n] && end > start && end < to)
                                    ++next;
                                sequence[next] |= (bit * 2);
                            }
                        }
                    }
                }
            }
        } else {
            TVector<TVector<std::pair<unsigned, TUtf16String> > >
                sequence(suffixStart - prefixEnd + 1);
            sequence[0].push_back(std::make_pair(0, TUtf16String()));
            for (size_t cur = 0; cur < sequence.size(); ++cur) {
                for (size_t n = 0; n < sequence[cur].size(); ++n) {
                    unsigned idx = sequence[cur][n].first;
                    IPhraseRecognizer *part = Parts[idx];
                    size_t start = prefixEnd + cur;
                    if (idx == Parts.size() - 1) {
                        TUtf16String w;
                        if (part->CollectMatch(phrase, start, suffixStart, &w)) {
                            *res = sequence[cur][n].second + w;
                            return true;
                        }
                    } else {
                        for (size_t end = start; end <= suffixStart; ++end) {
                            if (Spaces[idx] && end > start && end < to && phrase[end] != ' ')
                                continue;
                            size_t next = end - prefixEnd;
                            if (Spaces[n] && end > start && end < to)
                                ++next;
                            if (HasInSequence(sequence[next], idx + 1))
                                continue;
                            TUtf16String w;
                            if (part->CollectMatch(phrase, start, end, &w))
                                sequence[next].push_back(std::make_pair(idx + 1, sequence[cur][n].second + w));
                        }
                    }
                }
            }
        }
        return false;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsNonTrivial(const TVector<TUtf16String> &part) {
    Y_ASSERT(!part.empty());
    return part.size() > 1 || !part[0].empty();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool GetOneString(IPhraseRecognizer *ph, TUtf16String *res)
{
    TVector<TUtf16String> all;
    if (!ph->AddAllMatches(&all))
        return false;
    if (all.size() == 1) {
        *res = all[0];
        return true;
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* Sequence(const TVector<TPtr<IPhraseRecognizer> > &parts, const TVector<bool> &needsSpace)
{
    // 1) собираем вместе финитные куски
    TVector<IPhraseRecognizer*> realParts;
    TVector<bool> realNeedSpace;
    TVector<TUtf16String> curFinitePart;
    curFinitePart.push_back(TUtf16String());
    for (size_t n = 0; n < parts.size(); ++n) {
        TVector<TUtf16String> add;
        if (parts[n]->AddAllMatches(&add)) {
            size_t multSize = curFinitePart.size() * add.size();
            if (multSize <= TFiniteSetRecognizer::SIZE_THRESHOLD) {
                TVector<TUtf16String> newFinitePart(multSize);
                for (size_t m = 0; m < curFinitePart.size(); ++m) {
                    for (size_t k = 0; k < add.size(); ++k) {
                        TUtf16String &dst = newFinitePart[add.size() * m + k];
                        dst = curFinitePart[m];
                        if (n > 0 && needsSpace[n - 1] && !dst.empty() && dst.back() != ' ')
                            dst += ' ';
                        dst += add[k];
                    }
                }
                std::swap(newFinitePart, curFinitePart);
            } else {
                if (IsNonTrivial(curFinitePart)) {
                    realParts.push_back(CreateRecognizer(curFinitePart));
                    Y_ASSERT(n > 0);
                    realNeedSpace.push_back(needsSpace[n - 1]);
                }
                std::swap(curFinitePart, add);
            }
        } else {
            if (IsNonTrivial(curFinitePart)) {
                realParts.push_back(CreateRecognizer(curFinitePart));
                Y_ASSERT(n > 0);
                realNeedSpace.push_back(needsSpace[n - 1]);
                curFinitePart.clear();
                curFinitePart.push_back(TUtf16String());
            }
            realParts.push_back(parts[n]);
            realNeedSpace.push_back(needsSpace[n]);
        }
    }
    if (IsNonTrivial(curFinitePart)) {
        realParts.push_back(CreateRecognizer(curFinitePart));
        realNeedSpace.push_back(needsSpace[parts.size() - 1]);
    }
    // 2) если то, что получилось, финитно, то всё понятно
    Y_ASSERT(realParts.size() == realNeedSpace.size());
    if (realParts.empty())
        return new TEmptyRecognizer();
    if (realParts.size() == 1)
        return realParts[0];
    // 3) если начало или конец состоят из единственной фразы - используем это
    TSequenceRecognizer *ret = new TSequenceRecognizer;
    size_t startRec = 0;
    if (GetOneString(realParts.back(), &ret->Suffix)) {
        if (realNeedSpace.back())
            ret->Suffix = u" " + ret->Suffix;
        realParts.pop_back();
    }
    if (GetOneString(realParts[0], &ret->Prefix)) {
        if (realNeedSpace[0])
            ret->Prefix += ' ';
        startRec = 1;
    }
    // остальное запихиваем как есть
    for (size_t n = startRec; n < realParts.size(); ++n) {
        ret->Parts.push_back(realParts[n]);
        ret->Spaces.push_back(realNeedSpace[n]);
    }
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* ChangeDeclension(TPtr<IPhraseRecognizer> r, const TString &declensionName)
{
    EGrammar declension = TGrammarIndex::GetCode(declensionName);
    if (declension == gInvalid)
        ythrow yexception() << "unknown declension name: " << declensionName;
    TVector<TUtf16String> all;
    if (!r->AddAllMatches(&all))
        ythrow yexception() << "cannot change declension: recognizer is of non-finite type";
    TVector<TUtf16String> modified;
    for (size_t n = 0; n < all.size(); ++n)
        PhraseToDeclension(all[n], declension, &modified);
    return CreateRecognizer(modified);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TOptionalRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TOptionalRecognizer);
public:
    TPtr<IPhraseRecognizer> Recognizer;

    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        return from == to || Recognizer->CollectMatch(phrase, from, to, res);
    }
    bool AddAllMatches(TVector<TUtf16String> *all) const override {
        if (Recognizer->AddAllMatches(all)) {
            all->push_back(TUtf16String());
            return true;
        }
        return false;
    }
 };
IPhraseRecognizer* Optional(TPtr<IPhraseRecognizer> r) {
    if (r->IsMatch(TUtf16String(), 0, 0))
        return r;
    TVector<TUtf16String> matches;
    if (r->AddAllMatches(&matches)) {
        matches.push_back(TUtf16String());
        return CreateRecognizer(matches);
    }
    TOptionalRecognizer *ret = new TOptionalRecognizer;
    ret->Recognizer = r;
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsSingleWord(const TUtf16String &phrase, size_t from, size_t to)
{
    if (to == from)
        return false;
    for (size_t cur = from; cur < to; ++cur)
        if (phrase[cur] == ' ')
            return false;
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TMatchAnyRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TMatchAnyRecognizer);
    bool SingleWord;
public:
    TMatchAnyRecognizer(bool singleWord = false)
        : SingleWord(singleWord)
    {
    }
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        if (SingleWord && !IsSingleWord(phrase, from, to))
            return false;
        if (res)
            *res = phrase.substr(from, to - from);
        return true;
    }
};
IPhraseRecognizer* CreateMatchAnything(bool singleWord)
{
    return new TMatchAnyRecognizer(singleWord);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TMatchPatternRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TMatchPatternRecognizer);
    TUtf16String Prefix, Suffix;
    bool SingleWord;
public:
    TMatchPatternRecognizer()
    {
    }
    TMatchPatternRecognizer(const TUtf16String &prefix, const TUtf16String &suffix, bool singleWord)
        : Prefix(prefix)
        , Suffix(suffix)
        , SingleWord(singleWord)
    {
    }
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        size_t prefixEnd = from + (Prefix.size());
        if (prefixEnd > to || !IsSubstring(phrase, from, prefixEnd, Prefix))
            return false;
        size_t suffixStart = to - (Suffix.size());
        if (suffixStart < prefixEnd || !IsSubstring(phrase, suffixStart, to, Suffix))
            return false;
        if (SingleWord && !IsSingleWord(phrase, from, to))
            return false;
        if (res)
            *res = phrase.substr(from, to - from);
        return true;
    }
};
IPhraseRecognizer* CreateMatchPattern(const TUtf16String &prefix, const TUtf16String &suffix, bool singleWord)
{
    return new TMatchPatternRecognizer(prefix, suffix, singleWord);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TRangeRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TRangeRecognizer);
    wchar16 From;
    wchar16 To;
    TString AdditionalChars;
    bool AcceptSeveralWords;
public:
    TRangeRecognizer()
    {
    }
    TRangeRecognizer(wchar16 from, wchar16 to, const TString &additional, bool acceptSeveralWords)
        : From(from)
        , To(to)
        , AdditionalChars(additional)
        , AcceptSeveralWords(acceptSeveralWords)
    {
    }
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        if (to == from)
            return false;
        for (size_t n = from; n < to; ++n) {
            wchar16 c = phrase[n];
            if (AcceptSeveralWords && c == ' ')
                continue;
            else if (c >= From && c <= To)
                continue;
            else if (std::find(AdditionalChars.begin(), AdditionalChars.end(), c) != AdditionalChars.end())
                continue;
            return false;
        }
        if (res)
            *res = phrase.substr(from, to - from);
        return true;
    }
};
IPhraseRecognizer* CreateNumbersRecognizer(bool acceptSeveralWords)
{
    return new TRangeRecognizer('0', '9', "", acceptSeveralWords);
}
IPhraseRecognizer* CreateLatinicRecognizer(bool acceptSeveralWords)
{
    return new TRangeRecognizer('a', 'z', "+$%#@`'", acceptSeveralWords);
}

struct TGramRecognizer: public IPhraseRecognizer
{
    OBJECT_METHODS(TGramRecognizer);
    TGramBitSet Pattern;
public:
    TGramRecognizer()
    {
    }
    TGramRecognizer(const TString& pattern)
        : Pattern(TGramBitSet::FromString(pattern))
    {
    }
    TGramRecognizer(const TGramBitSet& pattern)
        : Pattern(pattern)
    {
    }
    bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const override {
        Y_UNUSED(res);
        if (!IsSingleWord(phrase, from, to))
            return false;
        TUtf16String word = phrase.substr(from, to - from);
        return IsMatchGramPattern(word, Pattern);
    }
};

IPhraseRecognizer* CreateGramRecognizer(const TString& pattern) {
    return new TGramRecognizer(pattern);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//BASIC_REGISTER_CLASS(IPhraseRecognizer);
BASIC_REGISTER_CLASS(IRecognizerFactory);
