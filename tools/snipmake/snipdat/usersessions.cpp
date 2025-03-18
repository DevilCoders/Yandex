#include "common.h"
#include "usersessions.h"

#include <util/generic/strbuf.h>
#include <util/string/split.h>
#include <util/string/cast.h>

namespace NSnippets {
    struct TUrlFieldsConsumer {
        TSessionRes* Res;
        bool Consume(const TStringBuf& key, const TStringBuf& value) {
            if (Equals(key, "url")) {
                Res->Url = value;
            } else if (Equals(key, "source")) {
                Res->Source = value;
            } else if (Equals(key, "num")) {
                Res->Num = FromString<int>(value);
            }
            return true;
        }
    };
    struct TUrlConsumer {
        TSessionEntry* Res;
        bool Consume(const char* beg, const char* end, const char*) {
            Res->Res.push_back(TSessionRes());
            TCharDelimiter<const char> del('\t');
            TUrlFieldsConsumer cons = { &Res->Res.back() };
            TConvertConsumer<TUrlFieldsConsumer> conv(&cons);
            SplitString(beg, end, del, conv);
            return true;
        }
    };
    struct TSessionConsumer {
        TSessionEntry* Res;
        int Idx;
        TSessionConsumer(TSessionEntry* res)
          : Res(res)
          , Idx(0)
        {
        }
        bool Consume(const TStringBuf& key, const TStringBuf& value) {
            if (key == "type") {
                Res->Type = value;
            } else if (key == "service") {
                Res->Service = value;
            } else if (key == "ui") {
                Res->UI = value;
            } else if (key == "uil") {
                Res->Uil = value;
            } else if (key == "query") {
                Res->Query = value;
            } else if (key == "dom-region") {
                Res->DomRegion = value;
            } else if (Equals(key, "user-region")) {
                Res->UserRegion = value;
            } else if (Equals(key, "test-buckets")) {
                Res->TestBuckets = value;
            } else if (Equals(key, "exp_config_version")) {
                Res->ExpConfigVersion = value;
            } else if (Equals(key, "full-request")) {
                Res->FullRequest = value;
            } else if (Equals(key, "page-size")) {
                Res->PageSize = FromString<int>(value);
            } else if (Equals(key, "page")) {
                Res->Page = FromString<int>(value);
            } else if (Equals(key, "num-docs")) {
                Res->NumDocs = FromString<int>(value);
            } else if (Equals(key, "") && Idx == 1) {
                Res->Timestamp = value;
            }
            ++Idx;
            return true;
        }
    };
    void ParseMREntry(TSessionEntry& res, const TString& tmp) {
        size_t tt = tmp.find("\t\t");
        if (tt == TString::npos) {
            tt = tmp.size();
        }
        TStringBuf left(tmp.data(), tmp.data() + tt);
        TCharDelimiter<const char> lDel('\t');
        TSessionConsumer lCons(&res);
        TConvertConsumer<TSessionConsumer> lConv(&lCons);
        SplitString(left.data(), left.data() + left.size(), lDel, lConv);

        if (res.Type == "REQUEST" && tt + 2 <= tmp.size()) {
            TStringBuf right(tmp.data() + tt + 2, tmp.data() + tmp.size());
            TStringDelimiter<const char> rDel("\t\t");
            TUrlConsumer rCons = { &res };
            SplitString(right.data(), right.data() + right.size(), rDel, rCons);
        }

    }
}
