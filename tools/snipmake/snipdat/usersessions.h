#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NSnippets {
    struct TSessionRes {
        TString Url;
        TString Source;
        int Num;
    };
    struct TSessionEntry {
        TString Timestamp;
        TString Type;
        TString Service;
        TString UI;
        TString Uil;
        TString Query;
        TString DomRegion;
        TString UserRegion;
        TString TestBuckets;
        TString ExpConfigVersion;
        TString FullRequest;
        int PageSize;
        int Page;
        int NumDocs;
        TVector<TSessionRes> Res;
        void Clear() {
            Timestamp.clear();
            Type.clear();
            Service.clear();
            UI.clear();
            Uil.clear();
            Query.clear();
            DomRegion.clear();
            UserRegion.clear();
            TestBuckets.clear();
            ExpConfigVersion.clear();
            FullRequest.clear();
            PageSize = 0;
            Page = 0;
            NumDocs = 0;
            Res.clear();
        }
    };
    void ParseMREntry(TSessionEntry& res, const TString& tmp);

    inline bool ParseNextMREntry(TSessionEntry& res, IInputStream* in) {
        TString tmp;
        try {
            res.Clear();
            if (!in->ReadLine(tmp)) {
                return false;
            }
            ParseMREntry(res, tmp);
            return true;
        } catch (...) {
            res.Clear();
            return true;
        }
    }
}
