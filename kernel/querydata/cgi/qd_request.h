#pragma once

#include "qd_docitems.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/vector.h>

namespace NQueryData {

    class TRequestRec {
    public:
        TSet<TString> FilterNamespaces;
        TSet<TString> FilterFiles;
        TSet<TString> SkipNamespaces;
        TSet<TString> SkipFiles;

        TString UserQuery;
        TString DoppelNorm;
        TString DoppelNormW;
        TString StrongNorm;

        TString UserId;
        TString UserLogin;
        TString UserLoginHash;
        TString UILang;
        TString YandexTLD;
        TString SerpType;

        TVector<TString> UserRegions;
        TVector<TString> UserRegionsIpReg;
        TString UserRegionsIpCountry;
        TVector<TString> UserRegionsIpRegDebug;
        TString UserIpMobileOp;
        TString UserIpMobileOpDebug;
        bool BegemotFailed = false;

        NSc::TValue StructKeys;
        TVector<TString> BinaryKeys;

        TString ReqId;
        bool IgnoreText = false;

        bool IsRecommendationsRequest = false;

        TDocItemsRec DocItems;

        NSc::TValue Other; // for overriding

    public:
        const TString& GetUserIpMobileOp() const {
            return UserIpMobileOpDebug ? UserIpMobileOpDebug : UserIpMobileOp;
        }

        const TVector<TString>& GetUserRegionsIpReg() const {
            return UserRegionsIpRegDebug ? UserRegionsIpRegDebug : UserRegionsIpReg;
        }

    public:

        NSc::TValue ToJson() const;
        static THolder<TRequestRec> FromJson(const NSc::TValue&);
    };

    void AdaptQueryDataRequest(TRequestRec&, const NSc::TValue&);
    void AdaptQueryDataRequest(NSc::TValue&, const TRequestRec&);

}
