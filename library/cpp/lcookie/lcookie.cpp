#include "lcookie.h"

#include <util/generic/vector.h>
#include <util/generic/singleton.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NLCookie {
    const char QuickUnpackReverseMap[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    struct TQuickUnpack {
        TQuickUnpack() {
            Map.assign(256, -1);
            for (size_t i = 0; i < Y_ARRAY_SIZE(QuickUnpackReverseMap) - 1; ++i) {
                Map[QuickUnpackReverseMap[i]] = i;
            }
        }

        TVector<char> Map;
    };

    const char* ERROR_MESSAGES[] = {
        "",
        "bad format",
        "key not found",
        "bad signature",
        "bad index",
        "bad value"};

    bool TryParse(TLCookie& cookie, const TStringBuf& l, const IKeychain& keychain, bool checkSign) {
        TStringBuf leftover = l;
        TStringBuf packed = leftover.NextTok('.');
        TStringBuf timestamp = leftover.NextTok('.');
        TStringBuf keyid = leftover.NextTok('.');
        TStringBuf salt = leftover.NextTok('.');
        TStringBuf sign = leftover.NextTok('.');

        (void)salt;

        if (sign.empty() || !leftover.empty()) {
            return false;
        }

        if (!TryFromString<ui32>(timestamp, cookie.Timestamp)) {
            return false;
        }

        TStringBuf key = keychain.GetKey(keyid);

        if (key.empty()) {
            return false;
        }

        if (checkSign) {
            const size_t digest_size = 32;
            char digest[digest_size + 1];
            leftover = l.Head(l.length() - sign.length() - 1);
            MD5 hash;
            hash.Update(leftover.data(), leftover.length());
            hash.Update(key.data(), key.length());
            hash.End(digest);
            if (sign != TStringBuf(digest, digest_size)) {
                return false;
            }
        }

        TVector<char> unpacked(Base64DecodeBufSize(packed.length()));
        unpacked.resize(Base64Decode(unpacked.begin(), packed.begin(), packed.end()));

        for (size_t i = 0; i < unpacked.size(); ++i) {
            // TODO: Optimize? Would it be much faster if we XORed INTs instead of CHARs?
            unpacked[i] ^= key[i];
        }

        const TVector<char>& qmap = Singleton<TQuickUnpack>()->Map;
        const int uid_offset = 4;
        const int login_offset = 24;
        int uid_off, uid_len, log_off, log_len;
        if ((uid_off = qmap[unpacked[0]]) == -1 ||
            (uid_len = qmap[unpacked[1]]) == -1 ||
            (log_off = qmap[unpacked[2]]) == -1 ||
            (log_len = qmap[unpacked[3]]) == -1)
        {
            return false;
        }

        TStringBuf uid(&unpacked[uid_offset + uid_off], uid_len);
        if (!TryFromString<ui64>(uid, cookie.Uid)) {
            return false;
        }

        cookie.Login.assign(&unpacked[login_offset + log_off], log_len);

        return true;
    }

}
