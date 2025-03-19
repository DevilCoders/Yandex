#include "qd_parser_utils.h"

#include <kernel/querydata/common/qd_key_token.h>

#include <util/generic/yexception.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NQueryData {

    bool QueryIsCommon(TStringBuf q) {
        return GetAllSubkeysCounts(q).Nonempty == 0;
    }

    TStringBuf SkipN(TStringBuf& data, ui32 n, char sep) {
        if (!n) {
            return "";
        }

        size_t off = -1;
        for (ui32 i = 0; i < n; ++i) {
            off = data.find(sep, off + 1);
            if (TStringBuf::npos == off) {
                TStringBuf res = data;
                data = "";
                return res;
            }
        }

        TStringBuf res = data.SubStr(0, off);
        data = data.SubStr(off + 1);
        return res;
    }

    static inline bool HasBinaryNormalization(int kt, const TKeyTypes& skts) {
        if (IsBinaryNormalization(kt)) {
            return true;
        }

        for (auto skt : skts) {
            if (IsBinaryNormalization(skt)) {
                return true;
            }
        }

        return false;
    }

    static inline void DecodeSubKey(TStringBuf& key, int kt, TString& keybuf) {
        TStringBuf sk = key.NextTok('\t');
        if (IsBinaryNormalization(kt)) {
            const size_t sz = keybuf.size();
            keybuf.ReserveAndResize(sz + Base64DecodeBufSize(sk.size()));
            keybuf.resize(sz + Base64Decode(sk, sz + keybuf.begin()).size());
        } else {
            keybuf.append(sk);
        }
    }

    TStringBuf UnescapeKey(TStringBuf key, int keytype, const TKeyTypes& subkeytypes, TString& keybuf) {
        if (HasBinaryNormalization(keytype, subkeytypes)) {
            keybuf.clear();
            DecodeSubKey(key, keytype, keybuf);

            for (auto skt : subkeytypes) {
                keybuf.append('\t');
                DecodeSubKey(key, skt, keybuf);
            }

            return keybuf;
        } else {
            return key;
        }
    }
}
