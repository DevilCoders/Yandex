#include "norm_util.h"

#include <kernel/hosts/owner/owner.h>

#include <util/generic/singleton.h>
#include <util/string/type.h>

namespace NQueryData {

    bool PushBackPooled(TStringBufs& subkeys, TDeque<TString>& pool, const TString& str) {
        if (!str) {
            return false;
        }

        pool.push_back(str);
        subkeys.push_back(pool.back());
        return true;
    }

    void GetTokens(TStringBufs& normquery, TStringBuf ss, char delim) {
        while (!!ss) {
            TStringBuf t = ss.NextTok(delim);
            if (!!t && !IsSpace(t)) {
                normquery.push_back(t);
            }
        }
    }

    void GetPairs(TStringBufs& normquery, TDeque<TString>& pool, TStringBuf s0, char delim) {
        TString s;

        while (!!s0) {
            TStringBuf first = s0.NextTok(delim);

            if (!first) {
                continue;
            }

            TStringBuf s1 = s0;

            while (!!s1) {
                TStringBuf second = s1.NextTok(delim);

                if (!second) {
                    continue;
                }

                s.clear();
                s.append(first).append(' ').append(second);
                PushBackPooled(normquery, pool, s);
            }

            normquery.push_back(first);
        }
    }

}
