#pragma once

#include "props.h"
#include "userid.h"

#include <library/cpp/scheme/scheme.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

// NOTE: constructors and factory functions here throw TBadArgumentException
//       in case of parse error.

namespace NUgc {
    // Class that represents update in UGC DB format
    class TUserUpdate {
    public:
        TUserUpdate(const TUserId& userId, const TStringBuf& updateId, TInstant updateTime,
                    const TStringBuf& appId, const NSc::TValue& updateData = NSc::TValue());

        const TUserId& GetUser() const;
        const TString& GetId() const;
        const TString& GetApp() const;
        const TInstant& GetDatabaseTime() const;
        const TInstant& GetFeedbackTime() const;

        const TMaybe<TUserId>& GetVisitor() const;
        void SetVisitor(const TUserId& visitor);

        const TMaybe<TString>& GetDeviceId() const;
        void SetDeviceId(const TString& deviceId);

        const TMaybe<TString>& GetCountryCode() const;
        void SetCountryCode(const TString& countryCode);

        const TMaybe<TString>& GetYandexTld() const;
        void SetYandexTld(const TString& yandexTld);

        // Gets json body used to save update in db.
        // It is used to store data field in db table.
        NSc::TValue GetDataJson() const;

        const TVirtualTables& GetTables() const;

        TVirtualTables& MutableTables();

        // Returns json for a single update in get-user-data format.
        void ToJson(NSc::TValue& out) const;
        NSc::TValue ToJson() const {
            NSc::TValue val;
            ToJson(val);
            return val;
        }

        static TUserUpdate FromJson(const NSc::TValue& json);
        static TUserUpdate FromDataJson(const TString& data);
        static TUserUpdate FromDataJson(const NSc::TValue& tval);

    private:
        TUserId User;
        TMaybe<TUserId> Visitor;
        TString Id;
        TInstant DatabaseTime;
        TInstant FeedbackTime; // mey be different from db time if we import old feedback
        TString App;
        TMaybe<TString> DeviceId;
        TVirtualTables Tables;
        TMaybe<TString> CountryCode;
        TMaybe<TString> YandexTld;
    };

} // namespace NUgc
