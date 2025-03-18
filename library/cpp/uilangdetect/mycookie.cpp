#include <util/generic/yexception.h>
#include <library/cpp/string_utils/base64/base64.h>

#include "mycookie.h"

TMyCookie::TMyCookie(const TString& myCookie) {
    Parse(myCookie);
}

bool TMyCookie::Parse(const TString& myCookie) {
    try {
        TAutoPtr<TMyCookieHash> blocksHash = ReadCookie(myCookie);
        blocksHash->swap(BlocksHash);
        return true;
    } catch (const yexception&) {
    }
    return false;
}

TAutoPtr<TMyCookie::TMyCookieHash> TMyCookie::ReadCookie(const TString& myCookie) const {
    TAutoPtr<TMyCookieHash> blocksHash(new TMyCookieHash);
    TString decoded = DecodeCookie(myCookie);
    TStringBuf s(decoded.data(), decoded.size());
    while (s.length() > 0) {
        TMyCookieBlock block;
        int blockId = ReadBlock(s, block);
        if (block.size() > 0)
            (*blocksHash)[blockId] = block;
    }
    return blocksHash;
}

size_t TMyCookie::GetValuesCount(int blockId) const {
    TMyCookieHash::const_iterator block = BlocksHash.find(blockId);
    if (block == BlocksHash.end())
        return 0;
    return block->second.size();
}

int TMyCookie::GetValue(int blockId, size_t valId) const {
    TMyCookieHash::const_iterator block = BlocksHash.find(blockId);
    if (block == BlocksHash.end() || block->second.size() <= valId)
        ythrow yexception() << "requested valN is too big";
    return block->second[valId];
}

TString TMyCookie::DecodeCookie(const TStringBuf& myCookie) const {
    TString decoded = Base64DecodeUneven(myCookie);
    if (decoded.size() == 0)
        ythrow yexception() << "empty cookie";
    if (decoded[0] != 0x63)
        ythrow yexception() << "incorrect cookie header";
    return decoded.substr(1, TString::npos);
}

int TMyCookie::ReadBlock(TStringBuf& s, TMyCookieBlock& block) const {
    block.clear();
    int blockId = ReadBlockId(s);
    if (blockId == 0)
        return 0;
    size_t valuesCount = ReadValuesCount(s);
    for (size_t i = 0; i < valuesCount; ++i)
        block.push_back(ReadValue(s));
    return blockId;
}

int TMyCookie::ReadBlockId(TStringBuf& s) const {
    if (s.length() == 0)
        ythrow yexception() << "broken cookie";
    int id = (ui8)s[0];
    s.Skip(1);
    return id;
}

size_t TMyCookie::ReadValuesCount(TStringBuf& s) const {
    if (s.length() == 0)
        ythrow yexception() << "broken cookie";
    size_t valuesCount = (ui8)s[0];
    s.Skip(1);
    return valuesCount;
}

int TMyCookie::ReadValue(TStringBuf& s) const {
    static const size_t bytesByCode[] = {1, 1, 2, 4};
    static const i32 maskByCode[4] = {0xff, 0xff, 0x3fff, 0x0fffffff};

    if (s.length() == 0)
        ythrow yexception() << "broken cookie";

    int code = (ui8)s[0] >> 6;
    size_t len = bytesByCode[code];
    int mask = maskByCode[code];

    if (len > s.length())
        ythrow yexception() << "broken cookie";

    int value = 0;
    for (size_t i = 0; i < len; ++i)
        value = (value << 8) | (ui8)s[i];

    s.Skip(len);
    return value & mask;
}
