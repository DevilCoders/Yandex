#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>
#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/new_engine/binary_dict/binary_dict.h>
#include <util/generic/string.h>

//#include "binary_dict/binary_dict.h"

using namespace NNewLemmer;

class TNewLemmer {
    friend class TExplorer;

public:
    typedef TBasicString<wchar16> TStringType;
    typedef TBasicStringBuf<wchar16> TBuffer;

    TNewLemmer(const char* data, size_t size, const NLemmer::TAlphabetWordNormalizer* alphaRules = nullptr);

    size_t Analyze(const TBuffer& word, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const;

    TString Fingerprint() const {
        return Data->GetFingerprint();
    }

    const IDictData& GetData() const {
        return *Data;
    }

    const NLemmer::TAlphabetWordNormalizer* GetAlphaRules() const {
        return AlphaRules;
    }

private:
    bool Append(const TBuffer& stem, TWLemmaArray& out, const IPatternPtr pattern, const IBlockPtr block, size_t prefixLen, size_t flexLen) const;
    bool IsValid(const TBuffer& word, size_t sourceSize, size_t endLen, const IPatternPtr& pattern) const;

public:
    static const wchar16 AffixDelimiter;
    static const wchar16 WordStartSymbol;
    static const wchar16 EmptyFlex[];

private:
    THolder<TBinaryDictData> Data;
    TString DataFingerprint;
    const NLemmer::TAlphabetWordNormalizer* AlphaRules;
};
