#include "update.h"

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

static TString GetResource(const TString& resourceName) {
    TString data;
    UNIT_ASSERT_NO_EXCEPTION_C(data = NResource::Find(resourceName), ". Resource name: " << resourceName);
    return data;
}

class TUserUpdateTest: public TTestBase {
    UNIT_TEST_SUITE(TUserUpdateTest);
    UNIT_TEST(ValidUserUpdateTest);
    UNIT_TEST(NotValidJsonUserUpdateTest);
    UNIT_TEST(NotValidTimeUpdateTest);
    UNIT_TEST(TimeIsStrUserUpdateTest);
    UNIT_TEST(EmptyIdAppParamsTest);
    UNIT_TEST(EmptyIdAppValuesTest);
    UNIT_TEST(WrongUserFormatTest);
    UNIT_TEST(UserJsonMismatchTest);
    UNIT_TEST(InvalidVisitorId);
    UNIT_TEST(ParsesDeviceId);
    UNIT_TEST(ThrowsOnNonStringDeviceId);
    UNIT_TEST(ParsesCountryCode);
    UNIT_TEST(ParsesYandexTld);
    UNIT_TEST(Serializes);
    UNIT_TEST(DifferentTimes);
    UNIT_TEST(NoTimeInData);
    UNIT_TEST(CheckFromDataJson);
    UNIT_TEST(CheckFromDataJsonUpdateFull);
    UNIT_TEST(CheckFromDataJsonUpdateWithoutApp);
    UNIT_TEST(CheckFromDataJsonUpdateWithoutUser);
    UNIT_TEST(CheckFromDataJsonUpdateWithoutTime);
    UNIT_TEST(CheckFromDataJsonUpdateWithoutUpdateId);
    UNIT_TEST(CheckFromDataJsonUpdateWithoutVersion);
    UNIT_TEST(CheckFromDataJsonUpdateWithoutType);
    UNIT_TEST(CheckFromDataJsonUpdateOnlyRequired);
    UNIT_TEST_SUITE_END();

public:
    TUserUpdateTest()
        : UpdateData(MakeUpdateData())
    {
    }

    void ValidUserUpdateTest() {
        UNIT_ASSERT_NO_EXCEPTION(NUgc::TUserUpdate("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", UpdateData));
    }

    void NotValidJsonUserUpdateTest() {
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", "not_valid_json"), TBadArgumentException);
    }

    void NotValidTimeUpdateTest() {
        auto val = UpdateData.Clone();
        val["time"].SetIntNumber(-1470756197000);
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val), TBadArgumentException);
    }

    void TimeIsStrUserUpdateTest() {
        auto val = UpdateData.Clone();
        val["time"] = "1470756197000";
        UNIT_ASSERT_NO_EXCEPTION(NUgc::TUserUpdate("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val));
    }

    void EmptyIdAppParamsTest() {
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("/user/666", "", TInstant::MilliSeconds(1470756197000), "", ""), TBadArgumentException);
    }

    void EmptyIdAppValuesTest() {
        auto val = UpdateData.Clone();
        val["updateId"] = "";
        val["app"] = "";
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val), TBadArgumentException);
    }

    void WrongUserFormatTest() {
        auto val = UpdateData.Clone();
        val["userId"] = "slowpoke";
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("slowpoke", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val), TBadArgumentException);
    }

    void UserJsonMismatchTest() {
        auto val = UpdateData.Clone();
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("/user/42", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val), TBadArgumentException);
    }

    void InvalidVisitorId() {
        auto val = UpdateData.Clone();
        val["visitorId"] = "invalid/visitor/123";
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val), TBadArgumentException);
    }

    void ParsesDeviceId() {
        auto val = UpdateData.Clone();
        val["deviceId"] = "/mobdevice/63AFD6C8";
        NUgc::TUserUpdate update("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val);
        UNIT_ASSERT(update.GetDeviceId());
        UNIT_ASSERT_STRINGS_EQUAL(*update.GetDeviceId(), "/mobdevice/63AFD6C8");
    }

    void ThrowsOnNonStringDeviceId() {
        auto val = UpdateData.Clone();
        val["deviceId"]["dict_field"] = 1;
        UNIT_ASSERT_EXCEPTION(NUgc::TUserUpdate("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val), TBadArgumentException);
    }

    void ParsesCountryCode() {
        auto val = UpdateData.Clone();
        val["countryCode"] = "169";
        NUgc::TUserUpdate update("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val);
        UNIT_ASSERT(update.GetCountryCode());
        UNIT_ASSERT_STRINGS_EQUAL(*update.GetCountryCode(), "169");
    }

    void ParsesYandexTld() {
        auto val = UpdateData.Clone();
        val["yandexTld"] = "by";
        NUgc::TUserUpdate update("/user/666", "req-123/3", TInstant::MilliSeconds(1470756197000), "serp", val);
        UNIT_ASSERT(update.GetYandexTld());
        UNIT_ASSERT_STRINGS_EQUAL(*update.GetYandexTld(), "by");
    }

    void Serializes() {
        auto val = UpdateData.Clone();
        val["visitorId"] = "/visitor/123";
        val["deviceId"] = "123";
        val["countryCode"] = "169";
        val["yandexTld"] = "by";
        NUgc::TUserUpdate update("/user/666", "req-123/3", TInstant::MilliSeconds(146), "serp", val);
        const NSc::TValue serializedUpdate = update.ToJson();
        UNIT_ASSERT_VALUES_EQUAL(serializedUpdate, NUgc::TUserUpdate::FromJson(serializedUpdate).ToJson());
        const TString str = serializedUpdate.ToJson();
        UNIT_ASSERT_STRING_CONTAINS(str, "deviceId");
        UNIT_ASSERT_STRING_CONTAINS(str, "123");
        UNIT_ASSERT_STRING_CONTAINS(str, "visitorId");
        UNIT_ASSERT_STRING_CONTAINS(str, "/visitor/123");
        UNIT_ASSERT_STRING_CONTAINS(str, "169");
        UNIT_ASSERT_STRING_CONTAINS(str, "by");

        UNIT_ASSERT_VALUES_EQUAL(serializedUpdate["time"].GetIntNumber(), 146);
        UNIT_ASSERT(serializedUpdate["value"].IsString());
        const NSc::TValue value = NSc::TValue::FromJsonThrow(serializedUpdate["value"].GetString());
        UNIT_ASSERT_VALUES_EQUAL(value["time"].GetIntNumber(), 1470756197000);
    }

    void DifferentTimes() {
        NUgc::TUserUpdate update("/user/666", "req-123/3", TInstant::MilliSeconds(42), "serp", UpdateData);
        UNIT_ASSERT_VALUES_EQUAL(update.GetFeedbackTime().MilliSeconds(), 1470756197000);
        UNIT_ASSERT_VALUES_EQUAL(update.GetDatabaseTime().MilliSeconds(), 42);
    }

    void NoTimeInData() {
        auto val = UpdateData.Clone();
        val.GetDictMutable().erase("time");
        NUgc::TUserUpdate update("/user/666", "req-123/3", TInstant::MilliSeconds(42), "serp", val);
        UNIT_ASSERT_VALUES_EQUAL(update.GetFeedbackTime().MilliSeconds(), 42);
    }

    void CheckFromDataJson() {
        auto val = UpdateData.Clone();
        NUgc::TUserUpdate update("/user/666", "req-123/3", TInstant::MilliSeconds(146), "serp", val);
        const NSc::TValue serializedUpdate = update.ToJson();
        NUgc::TUserUpdate userUpdate = NUgc::TUserUpdate::FromDataJson(TString(serializedUpdate["value"].GetString()));
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetUser().AsString(), "/user/666");
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetId(), "req-123/3");
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetApp(), "serp");
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetFeedbackTime().MilliSeconds(), 1470756197000);
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetDatabaseTime().MilliSeconds(), 1470756197000);
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetDataJson(), val);
    }

    void CheckFromDataJsonUpdateFull() {
        TString updateText = GetResource("/UpdateFull");
        NUgc::TUserUpdate userUpdate = NUgc::TUserUpdate::FromDataJson(updateText);
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetUser().AsString(), "/visitor/899");
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetVisitor()->AsString(), "/visitor/899");
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetId(), "serp/256/update-3");
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetApp(), "serp");
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetFeedbackTime().MilliSeconds(), 1510896907012);
        UNIT_ASSERT_VALUES_EQUAL(userUpdate.GetDatabaseTime().MilliSeconds(), 1510896907012);
    }

    void CheckFromDataJsonUpdateWithoutApp() {
        TString updateText = GetResource("/UpdateWithoutApp");
        UNIT_ASSERT_EXCEPTION_CONTAINS(NUgc::TUserUpdate::FromDataJson(updateText), TBadArgumentException, "No app in update");
    }

    void CheckFromDataJsonUpdateWithoutUser() {
        TString updateText = GetResource("/UpdateWithoutUserId");
        UNIT_ASSERT_EXCEPTION_CONTAINS(NUgc::TUserUpdate::FromDataJson(updateText), TBadArgumentException, "No userId in update");
    }

    void CheckFromDataJsonUpdateWithoutTime() {
        TString updateText = GetResource("/UpdateWithoutTime");
        UNIT_ASSERT_EXCEPTION_CONTAINS(NUgc::TUserUpdate::FromDataJson(updateText), TBadArgumentException, "No time in update");
    }

    void CheckFromDataJsonUpdateWithoutUpdateId() {
        TString updateText = GetResource("/UpdateWithoutUpdateId");
        UNIT_ASSERT_EXCEPTION_CONTAINS(NUgc::TUserUpdate::FromDataJson(updateText), TBadArgumentException, "No updateId in update");
    }

    void CheckFromDataJsonUpdateWithoutVersion() {
        TString updateText = GetResource("/UpdateWithoutVersion");
        UNIT_ASSERT_EXCEPTION_CONTAINS(NUgc::TUserUpdate::FromDataJson(updateText), TBadArgumentException, "Invalid version");
    }

    void CheckFromDataJsonUpdateWithoutType() {
        TString updateText = GetResource("/UpdateWithoutType");
        UNIT_ASSERT_EXCEPTION_CONTAINS(NUgc::TUserUpdate::FromDataJson(updateText), TBadArgumentException, "Invalid message type");
    }

    void CheckFromDataJsonUpdateOnlyRequired() {
        TString updateText = GetResource("/UpdateOnlyRequiredFields");
        UNIT_ASSERT_NO_EXCEPTION(NUgc::TUserUpdate::FromDataJson(updateText));
    }

private:
    static NSc::TValue MakeUpdateData() {
        NSc::TValue updateData;
        updateData["userId"] = "/user/666";
        updateData["updateId"] = "req-123/3";
        updateData["time"].SetIntNumber(1470756197000);
        updateData["app"] = "serp";
        updateData["type"] = "ugcupdate";
        updateData["version"] = "1.0";
        return updateData;
    }

private:
    const NSc::TValue UpdateData;
};

UNIT_TEST_SUITE_REGISTRATION(TUserUpdateTest);
