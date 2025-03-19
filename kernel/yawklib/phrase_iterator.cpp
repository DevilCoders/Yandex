#include "wtrutil.h"
#include <library/cpp/charset/wide.h>
#include <util/draft/holder_vector.h>
#include "phrase_analyzer.h"
#include "phrase_iterator.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseIterator* CreatePhraseIterator(const TUtf16String &templ, size_t from, size_t to, bool ignoreOptionalFragments);
////////////////////////////////////////////////////////////////////////////////////////////////////
class TPhraseIteratorOptional: public IPhraseIterator
{
    bool On;
    THolder<IPhraseIterator> Inner;

    bool Next() override {
        if (!On)
            On = true;
        else
            On = Inner->Next();
        return On;
    }

    TUtf16String Get() const override {
        if (On)
            return Inner->Get();
        return TUtf16String();
    }

    IPhraseRecognizer* CreateRecognizer(IRecognizerFactory *factory) const override {
        return Optional(Inner->CreateRecognizer(factory));
    }

public:
    TPhraseIteratorOptional(const TUtf16String &templ, size_t from, size_t to)
    {
        On = false;
        Inner.Reset(CreatePhraseIterator(templ, from, to, false));
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
static IPhraseRecognizer* RecognizerFromStr(const TUtf16String &str, IRecognizerFactory *factory)
{
    size_t subtractPos = str.find('\\');
    if (subtractPos != TUtf16String::npos)
        return Subtract(
            RecognizerFromStr(str.substr(0, subtractPos), factory),
            RecognizerFromStr(str.substr(subtractPos + 1), factory)
        );
    size_t declensionPos = str.find(u"->");
    if (declensionPos != TUtf16String::npos)
        return ChangeDeclension(
            RecognizerFromStr(str.substr(0, declensionPos), factory),
            WideToChar(str.substr(declensionPos + 2), CODES_YANDEX)
        );
    if (str[0] == '{' && str.back() == '}') {
        if (!factory)
            ythrow yexception() << "recognizer constructor found token " << str << ", but no factory is available";
        IPhraseRecognizer *ret = factory->Create(str.substr(1, str.size() - 2));
        if (!ret)
            ythrow yexception() << "cannot understand special token: " << str;
        return ret;
    }
    TVector<TUtf16String> matches;
    matches.push_back(str);
    return ::CreateRecognizer(matches);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class TPhraseIteratorWord: public IPhraseIterator
{
    TUtf16String Word;
    bool Next() override {
        return false;
    }
    TUtf16String Get() const override {
        return Word;
    }
    IPhraseRecognizer* CreateRecognizer(IRecognizerFactory *factory) const override {
        return RecognizerFromStr(Word, factory);
    }
public:
    TPhraseIteratorWord(const TUtf16String &templ, size_t from, size_t to)
        : Word(templ.substr(from, to - from))
    {
        for (wchar16* c = Word.begin(); c != Word.end(); ++c)
            if (*c == '_')
                *c = ' ';
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TPhraseIteratorSelect: public IPhraseIterator
{
    size_t Pos;
    THolderVector<IPhraseIterator> Parts;

    bool Next() override {
        if (Parts[Pos]->Next())
            return true;
        ++Pos;
        if (Pos == Parts.Size())
            Pos = 0;
        return Pos != 0;
    }

    TUtf16String Get() const override {
        return Parts[Pos]->Get();
    }

    IPhraseRecognizer* CreateRecognizer(IRecognizerFactory *factory) const override {
        TVector<TPtr<IPhraseRecognizer> > parts;
        for (size_t n = 0; n < Parts.Size(); ++n)
            parts.push_back(Parts[n]->CreateRecognizer(factory));
        return Union(parts);
    }

public:
    TPhraseIteratorSelect()
        : Pos(0)
    {
    }

    void AddPart(IPhraseIterator *part) {
        Parts.PushBack(part);
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class TPhraseIteratorSequence: public IPhraseIterator
{
    THolderVector<IPhraseIterator> Parts;
    TVector<bool> NeedsSpace;

    bool Next() override {
        size_t n = 0;
        while (n < Parts.Size() && !Parts[n]->Next())
            ++n;
        return n < Parts.Size();
    }

    TUtf16String Get() const override {
        TUtf16String ret;
        for (size_t n = 0; n < Parts.Size(); ++n) {
            ret += Parts[n]->Get();
            if (NeedsSpace[n] && ret.size() && ret.back() != ' ')
                ret += ' ';
        }
        return ret;
    }

    IPhraseRecognizer* CreateRecognizer(IRecognizerFactory *factory) const override {
        TVector<TPtr<IPhraseRecognizer> > parts;
        for (size_t n = 0; n < Parts.Size(); ++n)
            parts.push_back(Parts[n]->CreateRecognizer(factory));
        return Sequence(parts, NeedsSpace);
    }

public:
    void AddPart(IPhraseIterator *part) {
        Parts.PushBack(part);
        NeedsSpace.push_back(false);
    }
    void AddSpace() {
        NeedsSpace.back() = true;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// opt := '[' phrase ']'
// word := цифробуква+
// part := word | opt
// select := word | word '/' select
// phrase := select | select ' ' phrase
static IPhraseIterator* CreatePart(const TUtf16String &templ, size_t &pos, size_t to, bool ignoreOptionalFragments)
{
    size_t from = pos;
    if (templ[pos] == '[') {
        int level = 1;
        ++pos;
        while (level > 0 && pos < to) {
            if (templ[pos] == '[')
                ++level;
            else if (templ[pos] == ']')
                --level;
            ++pos;
        }
        if (level > 0)
            return nullptr;
        if (ignoreOptionalFragments)
            return new TPhraseIteratorWord(TUtf16String(), 0, 0);
        else
            return new TPhraseIteratorOptional(templ, from + 1, pos - 1);
    } else {
        while (pos < to && templ[pos] != '/' && templ[pos] != '[' && !IsBlank(templ[pos]))
            ++pos;
        return new TPhraseIteratorWord(templ, from, pos);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static IPhraseIterator* CreateSelector(const TUtf16String &templ, size_t &pos, size_t to, bool ignoreOptionalFragments)
{
    IPhraseIterator* first = CreatePart(templ, pos, to, ignoreOptionalFragments);
    if (pos == to || templ[pos] != '/')
        return first;
    TPhraseIteratorSelect *res = new TPhraseIteratorSelect();
    res->AddPart(first);
    while (pos < to && !IsBlank(templ[pos])) {
        ++pos;
        res->AddPart(CreatePart(templ, pos, to, ignoreOptionalFragments));
    }
    return res;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool SkipBlanks(const TUtf16String &templ, size_t &pos, size_t to)
{
    if (pos >= to || !IsBlank(templ[pos]))
        return false;
    do {
        ++pos;
    } while (pos < to && IsBlank(templ[pos]));
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseIterator* CreatePhraseIterator(const TUtf16String &templ, size_t from, size_t to, bool ignoreOptionalFragments)
{
    size_t pos = from;
    IPhraseIterator* first = CreateSelector(templ, pos, to, ignoreOptionalFragments);
    bool blanks = SkipBlanks(templ, pos, to);
    if (pos == to)
        return first;
    TPhraseIteratorSequence *res = new TPhraseIteratorSequence();
    res->AddPart(first);
    if (blanks)
        res->AddSpace();
    while (pos < to) {
        res->AddPart(CreateSelector(templ, pos, to, ignoreOptionalFragments));
        if (SkipBlanks(templ, pos, to))
            res->AddSpace();
    }
    return res;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseIterator* CreatePhraseIterator(const TUtf16String &templ, bool ignoreOptionalFragments)
{
    return CreatePhraseIterator(templ, 0, templ.size(), ignoreOptionalFragments);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TestPhraseIterator(const char *pattern)
{
    TUtf16String testPattern(CharToWide(pattern, csYandex));
    THolder<IPhraseIterator> iter(CreatePhraseIterator(testPattern));
    int count = 0;
    do {
        TUtf16String answer = iter->Get();
        printf("%s\n", WideToChar(answer, CODES_YANDEX).data());
        ++count;
    } while (iter->Next());
    printf("%d total\n", count);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
