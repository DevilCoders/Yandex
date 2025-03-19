#pragma once

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/unicode/punycode/punycode.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/subst.h>

// NOTE: constructors factory and getter functions here throw TBadArgumentException
//       in case of parse error.

namespace NUgc {
    class TObjectId {
    public:
        static const TVector<TString> ValidPrefixes;

    public:
        TObjectId() = default;
        TObjectId(TStringBuf objectId);

        const TString& AsString() const;
        TString AsUgcdbKey() const;
        TString AsUgcdb2Key() const;

#define DECLARE_ACCESSORS_FOR_PREFIX(Namespace, Id, _Prefix)                                \
    bool Is##Namespace##Object() const {                                                    \
        return ObjectId.StartsWith(_Prefix);                                                \
    }                                                                                       \
    TString Get##Id() const {                                                               \
        TStringBuf id;                                                                      \
        Y_ENSURE_EX(TStringBuf(ObjectId).AfterPrefix(_Prefix, id), TBadArgumentException());\
        return ToString(id);                                                                \
    }                                                                                       \
    static TObjectId From##Namespace##Id(TStringBuf id) {                                   \
        return TObjectId(TStringBuilder() << _Prefix << id);                                \
    }                                                                                       \
    static const TString Get##Namespace##Prefix() {                                         \
        return _Prefix;                                                                     \
    }                                                                                       \

    DECLARE_ACCESSORS_FOR_PREFIX(Sprav, Permalink, "/sprav/")
    DECLARE_ACCESSORS_FOR_PREFIX(Ontodb, Ontoid, "/ontoid/")
    DECLARE_ACCESSORS_FOR_PREFIX(Site, Id, "/site/")
    DECLARE_ACCESSORS_FOR_PREFIX(Market, Market, "/market/")
    DECLARE_ACCESSORS_FOR_PREFIX(MarketModel, MarketModel, "/market/model/")
    DECLARE_ACCESSORS_FOR_PREFIX(MarketShop, MarketShop, "/market/shop/")
    DECLARE_ACCESSORS_FOR_PREFIX(TravelOrg, TravelOrg, "/travel_org/")
    DECLARE_ACCESSORS_FOR_PREFIX(OrgChain, OrgChainPermalink, "/org-chain/")
    DECLARE_ACCESSORS_FOR_PREFIX(MiniApp, MiniApp, "/miniapp/")
    DECLARE_ACCESSORS_FOR_PREFIX(Offer, Offer, "/offer/")
    DECLARE_ACCESSORS_FOR_PREFIX(Education, EducationId, "/education/")
    DECLARE_ACCESSORS_FOR_PREFIX(Afisha, AfishaId, "/afisha/")
#undef DECLARE_ACCESSORS_FOR_PREFIX

    static TObjectId FromSpravPermalink(ui64 id) {
        return TObjectId(TStringBuilder() << "/sprav/" << id);
    }

    static TObjectId FromMarketShopId(ui64 id) {
        return TObjectId(TStringBuilder() << "/market/shop/" << id);
    }

    static TObjectId FromMarketModelId(ui64 id) {
        return TObjectId(TStringBuilder() << "/market/model/" << id);
    }

    static TObjectId FromSiteOwner(TStringBuf owner) {
        TString url = TString(owner);
        if (owner.Contains("xn--")) {
            url = ToString(PunycodeToHostName(owner));
        }
        TString encoded = Base64EncodeUrl(url);
        // python's urlsafe_b64encode which we use to generate object id for sites
        // uses '=' instead of ',' so we need to mimic this behavior
        SubstGlobal(encoded, ',', '=');
        return TObjectId(TStringBuilder() << "/site/" <<  encoded);
    }

    static TString AsSiteOwner(TObjectId id) {
        return Base64Decode(id.GetId());
    }

    private:
        TString ObjectId;
    };
} // namespace NUgc
