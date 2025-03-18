#include "oauth_token.h"

#include <library/cpp/digest/old_crc/crc.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/cast.h>

#include <algorithm>

namespace NAuthClientParser {
    bool TOAuthToken::Parse(TStringBuf token) {
        try {
            IsSucceed_ = ParseImpl(token);
        } catch (const std::exception&) { // impossible
            IsSucceed_ = false;
        }
        return IsSucceed_;
    }

    bool TOAuthToken::ParseImpl(TStringBuf token) {
        return FromEmbeddedV2(token) || FromEmbeddedV3(token) || FromStateless(token);
    }

    bool TOAuthToken::FromEmbeddedV2(TStringBuf token) {
        const size_t EMBEDDED_SIZE = 39;

        if (token.size() != EMBEDDED_SIZE || !IsBase64Url(token)) {
            return false;
        }

        char tmpBuf[EMBEDDED_SIZE + 1]; // + padding
        memcpy(tmpBuf, token.data(), EMBEDDED_SIZE);
        tmpBuf[EMBEDDED_SIZE] = '=';

        char decodedToken[Base64DecodeBufSize(EMBEDDED_SIZE)];
        Base64Decode(decodedToken, tmpBuf, tmpBuf + sizeof(tmpBuf));

        Uid_ = FromBytes(decodedToken + 1);

        return true;
    }

    bool TOAuthToken::FromEmbeddedV3(TStringBuf token) {
        const size_t PREFIX_SIZE = 3;
        const size_t EMBEDDED_SIZE = 55;

        if (token.size() != PREFIX_SIZE + EMBEDDED_SIZE ||
            token[0] != 'y' ||
            token[1] < '0' || token[1] > '4' ||
            token[2] != '_' ||
            !IsBase64Url(token.Skip(PREFIX_SIZE))) {
            return false;
        }

        char tmpBuf[EMBEDDED_SIZE + 1]; // + padding
        memcpy(tmpBuf, token.data(), EMBEDDED_SIZE);
        tmpBuf[EMBEDDED_SIZE] = '=';

        char decodedToken[Base64DecodeBufSize(EMBEDDED_SIZE)];
        Base64Decode(decodedToken, tmpBuf, tmpBuf + sizeof(tmpBuf));

        if (FromBytes(decodedToken + 37, 4) != crc32(decodedToken, 37)) {
            return false;
        }

        Uid_ = FromBytes(decodedToken + 1);

        return true;
    }

    bool TOAuthToken::FromStateless(TStringBuf token) {
        const char DELIMITER('.');

        const TStringBuf version = token.NextTok(DELIMITER);
        if (version != "1") {
            return false;
        }

        const TStringBuf uid = token.NextTok(DELIMITER);

        const TStringBuf clientId = token.NextTok(DELIMITER);
        const TStringBuf expires = token.NextTok(DELIMITER);
        const TStringBuf tokenId = token.NextTok(DELIMITER);
        const TStringBuf keyId = token.NextTok(DELIMITER);

        ui64 tmp;
        if (!TryFromString(clientId, tmp) ||
            !TryFromString(expires, tmp) ||
            !TryFromString(tokenId, tmp) ||
            !TryFromString(keyId, tmp))
        {
            return false;
        }

        const TStringBuf iv = token.NextTok(DELIMITER);
        const TStringBuf data = token.NextTok(DELIMITER);
        const TStringBuf tag = token.NextTok(DELIMITER);

        if (!iv || !IsBase64Url(iv) || !data || !IsBase64Url(data) || !tag || !IsBase64Url(tag)) {
            return false;
        }

        return TryFromString(uid, Uid_);
    }

    bool TOAuthToken::IsBase64Url(TStringBuf str) {
        return std::find_if_not(str.begin(), str.end(), [](const char c) {
                   return (c >= 'a' && c <= 'z') ||
                          (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') ||
                          c == '-' ||
                          c == '_';
               }) == str.end();
    }

    ui64 TOAuthToken::FromBytes(const char* bytes, size_t size) {
        ui64 result = 0;
        for (size_t idx = 0; idx < size; ++idx) {
            result <<= 8;
            result |= static_cast<unsigned char>(bytes[idx]);
        }
        return result;
    }
}
