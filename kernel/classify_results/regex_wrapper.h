#pragma once

#include <library/cpp/regex/pire/pire.h>

struct IBinSaver;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSerializibleScanner - wrapper, based on library/cpp/regex/pire/regexp.h for easy MR serialization
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TSerializibleScanner : public NPire::TScanner {
public:
    TSerializibleScanner();
    TSerializibleScanner(const NPire::TScanner& scanner);
    TSerializibleScanner(const TUtf16String& pattern);
    TSerializibleScanner(const TString& pattern, ECharset charset = CODES_UTF8);

    bool IsMatch(const TUtf16String& what) const;
    bool IsMatch(const TString& utf8what) const;

    int operator&(IBinSaver &f);
    bool operator == (const TSerializibleScanner& rhs) const;
};

TSerializibleScanner operator | (const TSerializibleScanner& lhs, const TSerializibleScanner& rhs);
void operator |= (TSerializibleScanner& lhs, const TSerializibleScanner& rhs);

typedef TSerializibleScanner TRxMatcher;
