#include "update.h"

#include <util/string/cast.h>

namespace NUgc {
    TUserUpdate::TUserUpdate(const TUserId& userId, const TStringBuf& updateId, TInstant updateTime,
                             const TStringBuf& appId, const NSc::TValue& updateData)
        : User(userId)
        , Id(updateId)
        , DatabaseTime(updateTime)
        , FeedbackTime(updateTime)
        , App(appId)
    {
        if (!updateData.IsNull()) {
            try {
                Tables = TVirtualTables::FromJson(updateData);
            } catch (...) {
                ythrow TBadArgumentException() << "Failed to deserialize tables data from json: "
                                          << CurrentExceptionMessage()
                                          << ". Json: " << updateData;
            }

            if (updateData.Has("time")) {
                const i64 feedbacktime = updateData["time"].ForceIntNumber();
                Y_ENSURE_EX(feedbacktime > 0,
                            TBadArgumentException() << "Invalid time: [" << feedbacktime << "].");
                FeedbackTime = TInstant::MilliSeconds(feedbacktime);
            } else {
                FeedbackTime = DatabaseTime;
            }

            Y_ENSURE_EX(updateData["updateId"].GetString() == Id,
                        TBadArgumentException() << "Update ids must match: [" << updateData["updateId"].GetString() << "] vs [" << Id << "].");
            Y_ENSURE_EX(updateData["app"].GetString() == App,
                        TBadArgumentException() << "Apps must match: [" << updateData["app"].GetString() << "] vs [" << App << "].");
            Y_ENSURE_EX(updateData["userId"].GetString() == User.AsString(),
                        TBadArgumentException() << "User ids must match: [" << updateData["userId"].GetString() << "] vs [" << User.AsString() << "].");
            Y_ENSURE_EX(updateData["type"].GetString() == TStringBuf("ugcupdate"),
                        TBadArgumentException() << "Invalid message type: [" << updateData["type"] << "].");
            Y_ENSURE_EX(updateData["version"].GetString() == TStringBuf("1.0"),
                        TBadArgumentException() << "Invalid version: [" << updateData["version"] << "].");

            {
                const NSc::TValue& visitor = updateData["visitorId"];
                if (!visitor.IsNull()) {
                    Y_ENSURE_EX(visitor.IsString(),
                                TBadArgumentException() << "Visitor must be string. Got [" << visitor << "] of type [" << visitor.GetType() << "].");
                    Visitor = visitor.GetString();
                }
            }

            {
                const NSc::TValue& deviceId = updateData["deviceId"];
                if (!deviceId.IsNull()) {
                    Y_ENSURE_EX(deviceId.IsString(),
                                TBadArgumentException() << "Device id must be string. Get [" << deviceId << "] of type [" << deviceId.GetType() << "].");
                    DeviceId = TString(deviceId.GetString());
                }
            }

            {
                const NSc::TValue& countryCode = updateData["countryCode"];
                if (!countryCode.IsNull()) {
                    Y_ENSURE_EX(countryCode.IsString(),
                                TBadArgumentException() << "CountryCode must be string. Get [" << countryCode << "] of type [" << countryCode.GetType() << "].");
                    CountryCode = TString(countryCode.GetString());
                }
            }

            {
                const NSc::TValue& yandexTld = updateData["yandexTld"];
                if (!yandexTld.IsNull()) {
                    Y_ENSURE_EX(yandexTld.IsString(),
                                TBadArgumentException() << "YandexTld must be string. Get [" << yandexTld << "] of type [" << yandexTld.GetType() << "].");
                    YandexTld = TString(yandexTld.GetString());
                }
            }
        }

        Y_ENSURE_EX(!Id.empty(),  TBadArgumentException() << "Id must be set.");
        Y_ENSURE_EX(!App.empty(), TBadArgumentException() << "App must be set.");
    }

    const TUserId& TUserUpdate::GetUser() const {
        return User;
    }

    const TMaybe<TUserId>& TUserUpdate::GetVisitor() const {
        return Visitor;
    }

    void TUserUpdate::SetVisitor(const TUserId& visitor) {
        Visitor = visitor;
    }

    const TString& TUserUpdate::GetId() const {
        return Id;
    }

    const TString& TUserUpdate::GetApp() const {
        return App;
    }

    const TInstant& TUserUpdate::GetDatabaseTime() const {
        return DatabaseTime;
    }

    const TInstant& TUserUpdate::GetFeedbackTime() const {
        return FeedbackTime;
    }

    const TMaybe<TString>& TUserUpdate::GetDeviceId() const {
        return DeviceId;
    }

    void TUserUpdate::SetDeviceId(const TString& deviceId) {
        DeviceId = deviceId;
    }

    const TMaybe<TString>& TUserUpdate::GetCountryCode() const {
        return CountryCode;
    }
    void TUserUpdate::SetCountryCode(const TString& countryCode) {
        CountryCode = countryCode;
    }

    const TMaybe<TString>& TUserUpdate::GetYandexTld() const {
        return YandexTld;
    }

    void TUserUpdate::SetYandexTld(const TString& yandexTld) {
        YandexTld = yandexTld;
    }

    NSc::TValue TUserUpdate::GetDataJson() const {
        NSc::TValue json = Tables.ToJson();
        json["type"] = "ugcupdate";
        json["version"] = "1.0";
        json["app"] = App;
        json["time"].SetIntNumber(static_cast<i64>(FeedbackTime.MilliSeconds()));
        json["updateId"] = Id;
        json["userId"] = User.AsString();
        if (Visitor) {
            json["visitorId"] = Visitor->AsString();
        }
        if (DeviceId) {
            json["deviceId"] = *DeviceId;
        }
        if (CountryCode) {
            json["countryCode"] = *CountryCode;
        }
        if (YandexTld) {
            json["yandexTld"] = *YandexTld;
        }
        return json;
    }

    const TVirtualTables& TUserUpdate::GetTables() const {
        return Tables;
    }

    TVirtualTables& TUserUpdate::MutableTables() {
        return Tables;
    }

    void TUserUpdate::ToJson(NSc::TValue& out) const {
        out["id"] = GetId();
        out["time"].SetIntNumber(DatabaseTime.MilliSeconds());
        out["app"] = GetApp();
        out["value"] = GetDataJson().ToJson();
    }

    TUserUpdate TUserUpdate::FromJson(const NSc::TValue& json) {
        const NSc::TValue updateData = NSc::TValue::FromJsonThrow(json["value"].GetString());
        return TUserUpdate(updateData["userId"].GetString(),
                           json["id"].GetString(),
                           TInstant::MilliSeconds(json["time"].GetIntNumber()),
                           json["app"].GetString(),
                           updateData);
    }

    TUserUpdate TUserUpdate::FromDataJson(const TString& data) {
        const NSc::TValue updateData = NSc::TValue::FromJsonThrow(data);
        return FromDataJson(updateData);
    }

    TUserUpdate TUserUpdate::FromDataJson(const NSc::TValue& updateData) {
        Y_ENSURE_EX(updateData.Has("userId"),   TBadArgumentException() << "No userId in update");
        Y_ENSURE_EX(updateData.Has("updateId"), TBadArgumentException() << "No updateId in update");
        Y_ENSURE_EX(updateData.Has("time"),     TBadArgumentException() << "No time in update");
        Y_ENSURE_EX(updateData.Has("app"),      TBadArgumentException() << "No app in update");

        return TUserUpdate(updateData["userId"].GetString(),
                           updateData["updateId"].GetString(),
                           TInstant::MilliSeconds(updateData["time"].GetIntNumber()),
                           updateData["app"].GetString(),
                           updateData);
    }
} // namespace NUgc
