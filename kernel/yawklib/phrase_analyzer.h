#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/ysafeptr.h>
#include <utility>
////////////////////////////////////////////////////////////////////////////////////////////////////
struct IPhraseRecognizer: public IObjectBase
{
protected:
    virtual bool IsMatchImpl(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const = 0;
public:
    bool IsMatch(const TUtf16String &phrase, size_t from, size_t to) const {
        Y_ASSERT(to >= from);
        Y_ASSERT(to <= phrase.size());
        return IsMatchImpl(phrase, from, to, nullptr);
    }
    bool CollectMatch(const TUtf16String &phrase, size_t from, size_t to, TUtf16String *res) const {
        Y_ASSERT(to >= from);
        Y_ASSERT(to <= phrase.size());
        return IsMatchImpl(phrase, from, to, res);
    }
    virtual bool AddAllMatches(TVector<TUtf16String>*) const {
        return false;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateRecognizer(const TVector<TUtf16String> &matches);
IPhraseRecognizer* Optional(TPtr<IPhraseRecognizer> r);
IPhraseRecognizer* Union(const TVector<TPtr<IPhraseRecognizer> > &parts);
IPhraseRecognizer* Sequence(const TVector<TPtr<IPhraseRecognizer> > &parts, const TVector<bool> &needsSpace);
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* Subtract(TPtr<IPhraseRecognizer> recognizer, TPtr<IPhraseRecognizer> stoplist);
IPhraseRecognizer* ChangeDeclension(TPtr<IPhraseRecognizer> r, const TString &declension); // see TGrammarIndex for probable names
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateMatchAnything(bool singleWord);
IPhraseRecognizer* CreateMatchPattern(const TUtf16String &prefix, const TUtf16String &suffix, bool singleWord);
IPhraseRecognizer* CreateNumbersRecognizer(bool acceptSeveralWords);
IPhraseRecognizer* CreateLatinicRecognizer(bool acceptSeveralWords);
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateTranslator(const TVector<std::pair<TUtf16String, TUtf16String> > &translations);
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseRecognizer* CreateGramRecognizer(const TString& pattern);
////////////////////////////////////////////////////////////////////////////////////////////////////

struct IRecognizerFactory: public IObjectBase {
    virtual IPhraseRecognizer* Create(const TUtf16String &description) = 0;
    virtual void GetUsedFileNames(TVector<TString>*) const {
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
