#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>

class TMyCookie {
public:
    TMyCookie(){};
    TMyCookie(const TString& myCookie);
    bool Parse(const TString& myCookie);
    size_t GetValuesCount(int blockId) const;
    int GetValue(int blockId, size_t valId) const;
    inline auto begin() {
        return BlocksHash.begin();
    };
    inline auto end() {
        return BlocksHash.end();
    };

private:
    typedef TVector<int> TMyCookieBlock;
    typedef THashMap<int, TMyCookieBlock> TMyCookieHash;

private:
    TString DecodeCookie(const TStringBuf& s) const;
    int ReadBlock(TStringBuf& s, TMyCookieBlock& block) const;
    int ReadBlockId(TStringBuf& s) const;
    size_t ReadValuesCount(TStringBuf& s) const;
    int ReadValue(TStringBuf& s) const;
    TAutoPtr<TMyCookieHash> ReadCookie(const TString& myCookie) const;

private:
    TMyCookieHash BlocksHash;
};
