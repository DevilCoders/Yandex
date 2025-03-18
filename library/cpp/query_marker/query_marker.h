#pragma once

#include <library/cpp/charset/codepage.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/charset/wide.h>
#include <util/stream/file.h>
#include <utility>

using TWordTypeDistr = TMap<int, size_t>;

template <class TStringType>
class TStringNormalizeImpl {
public:
    typedef typename TStringType::char_type TCharType;

    static TStringType FromFile(const TString& s);

    static bool IsAlphaNum(TCharType c);

    static bool IsApostrophe(TCharType c);

    static TCharType ToLower(TCharType c);
};

inline bool IsApostrophe(int c) {
    return c == '\'' || c == '`';
}

template <>
class TStringNormalizeImpl<TString> {
public:
    static TString FromFile(const TString& s) {
        return s;
    }

    static bool IsAlphaNum(char c) {
        return csYandex.IsAlnum((unsigned char)c);
    }

    static bool IsApostrophe(char c) {
        return ::IsApostrophe(int(c));
    }

    static char ToLower(char c) {
        return (char)csYandex.ToLower((unsigned char)c);
    }
};

template <>
class TStringNormalizeImpl<TUtf16String> {
public:
    static TUtf16String FromFile(const TString& s) {
        return UTF8ToWide<true>(s.data(), s.size());
    }

    static bool IsAlphaNum(wchar16 c) {
        return ::IsAlnum((wchar32)c);
    }

    static bool IsApostrophe(wchar16 c) {
        return ::IsApostrophe(int(c));
    }

    static wchar16 ToLower(wchar16 c) {
        return (wchar16)::ToLower((wchar32)c);
    }
};

template <class TStringType>
TStringType StringNormalize(const TString& str, bool changeApostropheToSpace = false) {
    typedef typename TStringType::char_type TCharType;

    TStringType s = TStringNormalizeImpl<TStringType>::FromFile(str);

    TVector<TCharType> v;
    v.reserve(s.size());
    bool prevIsAlphaNum = false;
    for (const TCharType* c = s.begin(); c != s.end(); ++c) {
        bool isApostrophe = TStringNormalizeImpl<TStringType>::IsApostrophe(*c);
        bool isAlphaNum = TStringNormalizeImpl<TStringType>::IsAlphaNum(*c) || (isApostrophe && !changeApostropheToSpace);
        if (isAlphaNum) {
            if (prevIsAlphaNum != isAlphaNum)
                v.push_back(' ');
            if (!isApostrophe)
                v.push_back(TStringNormalizeImpl<TStringType>::ToLower(*c));
        }
        prevIsAlphaNum = isAlphaNum;
    }
    v.push_back(' ');
    return TStringType(v.begin(), v.size());
}

template <class TStringType>
class TNormalizedStroka {
public:
    typedef typename TStringType::char_type TCharType;

private:
    TString Original;
    TStringType Normalized;
    TVector<ui32> TokenStart;
    TVector<ui32> TokenNStart;
    TMap<ui32, ui32> NormalizedMapping; // Position in Normalized -> Token Number

public:
    TNormalizedStroka(const TString& str);

    TStringType& GetNormalized() {
        return Normalized;
    }

    ui32 GetTokenPosition(ui32 t) const {
        return TokenStart[t];
    }

    ui32 GetTokenNPosition(ui32 t) const {
        return TokenNStart[t];
    }

    ui32 GetToken(ui32 pos) const {
        Y_ASSERT(NormalizedMapping.find(pos) != NormalizedMapping.end());
        return NormalizedMapping.find(pos)->second;
    }

    TStringType GetTokenText(ui32 tokenNumber) const {
        return Normalized.substr(TokenNStart[tokenNumber], TokenNStart[tokenNumber + 1] - TokenNStart[tokenNumber]);
    }

    TStringType& GetOriginal() {
        return Original;
    }

    ui32 Size() const {
        return TokenStart.size() - 1;
    }
};

struct TMarkerEntry {
    ui32 StartToken;
    ui32 EndToken;
    int Type;
    TString Text;

public:
    TMarkerEntry(ui32 startToken = 0, ui32 endToken = 0, int markerType = 0, const TString& markerText = "")
        : StartToken(startToken)
        , EndToken(endToken)
        , Type(markerType)
        , Text(markerText)
    {
    }
    int NumTokens() const {
        return EndToken - StartToken + 1;
    }
};

template <class TStringType>
TNormalizedStroka<TStringType>::TNormalizedStroka(const TString& s) {
    Original = s;
    TVector<TCharType> v;
    bool prevIsAlphaNum = false;
    int p = 0;
    int tokenNumber = 0;
    TStringType str = TStringNormalizeImpl<TStringType>::FromFile(s);
    for (const TCharType *c = str.begin(); c != str.end(); ++c, ++p) {
        bool isApostrophe = TStringNormalizeImpl<TStringType>::IsApostrophe(*c);
        bool isAlphaNum = TStringNormalizeImpl<TStringType>::IsAlphaNum(*c) || isApostrophe;
        if (isAlphaNum) {
            if (prevIsAlphaNum != isAlphaNum) {
                v.push_back(' ');
                TokenStart.push_back(p);
                TokenNStart.push_back(v.size() - 1);
                NormalizedMapping[v.size() - 1] = tokenNumber;
                ++tokenNumber;
            }
            if (!isApostrophe)
                v.push_back(TStringNormalizeImpl<TStringType>::ToLower(*c));
        }
        prevIsAlphaNum = isAlphaNum;
    }
    TokenStart.push_back(p);
    v.push_back(' ');
    TokenNStart.push_back(v.size() - 1);
    NormalizedMapping[v.size() - 1] = tokenNumber;
    Normalized = TStringType(v.begin(), v.size());
}

class IQueryMarker {
public:
    virtual ~IQueryMarker() {
    }

    // TODO: throw not implemented
    virtual void Load(const TString& /*fileName*/) {
    }
    virtual void Save(const TString& /*fileName*/) {
    }

    virtual bool LoadMarker(const TString& s, int wordType) = 0;
    virtual ui32 LoadMarkers(const TString& path, int wordType) = 0;
    virtual void LoadMarkers(const TString& path, const TMap<TString, int>& topicTypes, TWordTypeDistr& markersCount) = 0;
    virtual void FinishInit() = 0;

    virtual ui32 QueryClassify(const TString& query, TWordTypeDistr& typeCount) const = 0;
};
