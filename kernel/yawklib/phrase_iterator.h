#pragma once

#include <util/generic/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
struct IPhraseRecognizer;
struct IRecognizerFactory;
struct IPhraseIterator
{
    virtual bool Next() = 0;
    virtual TUtf16String Get() const = 0;
    virtual IPhraseRecognizer* CreateRecognizer(IRecognizerFactory *factory) const = 0;
    virtual ~IPhraseIterator() {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
IPhraseIterator* CreatePhraseIterator(const TUtf16String &templ, bool ignoreOptionalFragments = false);
void TestPhraseIterator(const char* pattern);
////////////////////////////////////////////////////////////////////////////////////////////////////
