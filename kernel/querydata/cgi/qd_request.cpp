#include "qd_request.h"

#include <kernel/querydata/idl/scheme/querydata_request.sc.h>
#include <kernel/querydata/cgi/qd_cgi_utils.h>

#include <library/cpp/scheme/domscheme_traits.h>

#include <util/generic/algorithm.h>

namespace NQueryData {

    void Merge(TVector<TString>& dst, const TVector<TString>& src) {
        dst.insert(dst.end(), src.begin(), src.end());
    }

    void Merge(TSet<TString>& dst, const TVector<TString>& src) {
        dst.insert(src.begin(), src.end());
    }

    TSet<TString> AsSet(const TVector<TString>& vs) {
        TSet<TString> res;
        res.insert(vs.begin(), vs.end());
        return res;
    }

#if defined(QD_ASSIGN) || defined(QD_ASSIGN_VALUES) || defined(QD_ASSIGN_KEYS_SET)
#   error "already defined"
#else
#   define QD_ASSIGN(name) dst.name = src.name();
#   define QD_ASSIGN_VALUES(name) dst.name = GetValues<TString>(src.name().GetRawValue()->GetArray());
#   define QD_ASSIGN_KEYS_SET(name) dst.name = AsSet(GetDictKeys<TString>(src.name().GetRawValue()->GetDict()));

    void AdaptQueryDataRequest(TRequestRec& dst, const NSc::TValue& val) {
        TQueryDataRequestConst<TSchemeTraits> src(&val);

        QD_ASSIGN(DoppelNorm);
        QD_ASSIGN(DoppelNormW);
        QD_ASSIGN(StrongNorm);
        QD_ASSIGN(UserQuery);
        QD_ASSIGN(YandexTLD);

        QD_ASSIGN(SerpType);
        QD_ASSIGN(UILang);
        QD_ASSIGN(UserId);
        QD_ASSIGN(UserLogin);
        QD_ASSIGN(UserLoginHash);
        QD_ASSIGN(UserIpMobileOp);
        QD_ASSIGN(UserIpMobileOpDebug);

        QD_ASSIGN_VALUES(UserRegions);
        QD_ASSIGN_VALUES(UserRegionsIpReg);
        QD_ASSIGN_VALUES(UserRegionsIpRegDebug);

        QD_ASSIGN_KEYS_SET(FilterNamespaces);
        QD_ASSIGN_KEYS_SET(FilterFiles);
        QD_ASSIGN_KEYS_SET(SkipNamespaces);
        QD_ASSIGN_KEYS_SET(SkipFiles);

        dst.DocItems.FromJson(*src.DocItems().GetRawValue());

        dst.StructKeys = src.StructKeys().GetRawValue()->Clone();
        dst.Other = src.Other().GetRawValue()->Clone();

        QD_ASSIGN(ReqId);
        QD_ASSIGN(IgnoreText);
    }

#   undef QD_ASSIGN
#   undef QD_ASSIGN_VALUES
#   undef QD_ASSIGN_KEYS_SET
#endif

    template <typename TColl>
    static void InsertKeys(NSc::TValue& v, const TColl& coll) {
        for (TStringBuf item : coll) {
            v.GetOrAdd(item) = 1;
        }
    }

    template <typename TColl>
    static void InsertArray(NSc::TValue& v, const TColl& coll) {
        for (TStringBuf item : coll) {
            v.Push().SetString(item);
        }
    }

#if defined(QD_ASSIGN) || defined(QD_ASSIGN_KEYS)
#   error "already defined"
#else
#   define QD_ASSIGN(name) if (src.name) { dst.name() = src.name; }
#   define QD_ASSIGN_KEYS(name) if (src.name) { InsertKeys(*dst.name().GetRawValue(), src.name); }

    void AdaptQueryDataRequest(NSc::TValue& val, const TRequestRec& src) {
        TQueryDataRequest<TSchemeTraits> dst(&val);

        QD_ASSIGN(DoppelNorm);
        QD_ASSIGN(DoppelNormW);
        QD_ASSIGN(StrongNorm);
        QD_ASSIGN(UserQuery);
        QD_ASSIGN(YandexTLD);

        QD_ASSIGN(SerpType);
        QD_ASSIGN(UILang);
        QD_ASSIGN(UserId);
        QD_ASSIGN(UserLogin);
        QD_ASSIGN(UserLoginHash);
        QD_ASSIGN(UserIpMobileOp);
        QD_ASSIGN(UserIpMobileOpDebug);

        QD_ASSIGN(UserRegions);
        QD_ASSIGN(UserRegionsIpReg);
        QD_ASSIGN(UserRegionsIpRegDebug);

        QD_ASSIGN_KEYS(FilterNamespaces);
        QD_ASSIGN_KEYS(FilterFiles);
        QD_ASSIGN_KEYS(SkipNamespaces);
        QD_ASSIGN_KEYS(SkipFiles);

        if (src.DocItems) {
            src.DocItems.ToJson(*dst.DocItems().GetRawValue());
        }

        if (!src.StructKeys.IsNull()) {
            *dst.StructKeys().GetRawValue() = src.StructKeys.Clone();
        }

        if (!src.Other.IsNull()) {
            *dst.Other().GetRawValue() = src.Other.Clone();
        }

        QD_ASSIGN(ReqId);

        dst.IgnoreText() = src.IgnoreText;
    }

#   undef QD_ASSIGN
#   undef QD_ASSIGN_KEYS
#endif

    NSc::TValue TRequestRec::ToJson() const {
        NSc::TValue res;
        AdaptQueryDataRequest(res, *this);
        return res;
    }

    THolder<TRequestRec> TRequestRec::FromJson(const NSc::TValue& val) {
        THolder<TRequestRec> rec{new TRequestRec};
        AdaptQueryDataRequest(*rec, val);
        return rec;
    }

}
