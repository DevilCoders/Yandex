#include <util/folder/dirut.h>
#include <util/stream/file.h>
#include <library/cpp/charset/wide.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <ysite/yandex/doppelgangers/normalize.h>
#include "wtrutil.h"
#include "factories.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateStdFactoryToken(const TString &tk, const TUtf16String &wtk)
{
    // yes, it's very stupid linear comparison
    // definitely this can be made 100 times faster
    // but this is absolutely not the place where speed matters
    // so just don't do it. :)
    if (tk.empty())
        return CreateRecognizer(TVector<TUtf16String>());
    if (tk == "*")
        return CreateMatchAnything(true);
    if (tk == "**")
        return CreateMatchAnything(false);
    if (tk == "123" || tk == "1234")
        return CreateNumbersRecognizer(false);
    if (tk == "123*" || tk == "1234*")
        return CreateNumbersRecognizer(true);
    if (tk == "ABC" || tk == "abc")
        return CreateLatinicRecognizer(false);
    if (tk == "ABC*" || tk == "abc*")
        return CreateLatinicRecognizer(true);
    size_t asterixPos = wtk.find('*');
    if (asterixPos != TUtf16String::npos) {
        if (wtk.find('*', asterixPos + 2) != TUtf16String::npos)
            return nullptr; // can't grok this pattern
        bool singleWord = true;
        size_t next = asterixPos + 1;
        if (asterixPos < wtk.size() - 1 && wtk[asterixPos + 1] == '*') {
            ++next;
            singleWord = false;
        }
        return CreateMatchPattern(wtk.substr(0, asterixPos), wtk.substr(next), singleWord);
    }
    return nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateFileToken(const TString &tk)
{
    if (NFs::Exists(tk)) {
        TDoppelgangersNormalize normalizer(false, false, false);
        TFileInput file(tk);
        TString line;
        TVector<TUtf16String> matches;
        while (file.ReadLine(line)) {
            TUtf16String wline = UTF8ToWide(line.data(), line.size(), csYandex);
            TVector<wchar16*> tabs;
            Wsplit(wline.begin(), '\t', &tabs);
            for (size_t n = 0; n < tabs.size(); ++n) {
                TUtf16String elem = normalizer.Normalize(tabs[n], false);
                if (!elem.empty())
                    matches.push_back(elem);
            }
        }
        return CreateRecognizer(matches);
    }
    return nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateGramToken(const TUtf16String &tk)
{
    TString pattern = WideToChar(tk, CODES_YANDEX);
    TGramBitSet bits = TGramBitSet::FromUnsafeString(pattern.data());
    if (bits.none())
        return nullptr;
    return CreateGramRecognizer(pattern);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
class TDefaultRecognizerFactory: public IRecognizerFactory
{
    OBJECT_METHODS(TDefaultRecognizerFactory);
    THashSet<TString> UsedFiles;
    TString FilesPath;
public:
    TDefaultRecognizerFactory() {
    }
    TDefaultRecognizerFactory(const TString &filesPath)
        : FilesPath(filesPath)
    {
    }
    IPhraseRecognizer* Create(const TUtf16String &tk) override {
        TUtf16String tkWithoutSpaces = tk;
        for (wchar16* c = tkWithoutSpaces.begin(); *c; ++c)
            if (*c == ' ')
                *c = '_';
        TString utfTk = WideToUTF8(tkWithoutSpaces);
        IPhraseRecognizer *ret;
        if ((ret = CreateStdFactoryToken(utfTk, tk)))
            return ret;
        if ((ret = CreateFileToken(FilesPath + utfTk))) {
            UsedFiles.insert(utfTk);
            return ret;
        }
        return nullptr;
    }
    void GetUsedFileNames(TVector<TString> *res) const override
    {
        for (THashSet<TString>::const_iterator it = UsedFiles.begin(); it != UsedFiles.end(); ++it)
            res->push_back(*it);
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
IRecognizerFactory *CreateDefaultRecognizerFactory()
{
    return new TDefaultRecognizerFactory();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
IRecognizerFactory *CreateDefaultRecognizerFactory(const TString &path)
{
    return new TDefaultRecognizerFactory(path);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
