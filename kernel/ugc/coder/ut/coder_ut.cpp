#include "coder.h"

#include <kernel/ugc/aggregation/feedback.h>

#include <library/cpp/testing/unittest/registar.h>

class TCoderTest: public TTestBase {
    UNIT_TEST_SUITE(TCoderTest)
    UNIT_TEST(EncodeCommonTest);
    UNIT_TEST(DecodeCommonTest);
    UNIT_TEST(EncodeArrayTest);
    UNIT_TEST(EncodeInnerArrayTest);
    UNIT_TEST(EncodeAlreadyEncodedTest);
    UNIT_TEST(DecodeAbsentFieldTest);
    UNIT_TEST(EmptyKeyTest);
    UNIT_TEST(EncodeDecodeSomeInnerArrays);
    UNIT_TEST(SizeTest);
    UNIT_TEST(CheckTokenInfo);
    UNIT_TEST(CheckTokenInfoFromSchemaObjects);
    UNIT_TEST(CheckTokenEmptyProfile);
    UNIT_TEST(CheckTokenEmptyProfileFromSchemaObjects);
    UNIT_TEST(CheckOType);
    UNIT_TEST(CheckNoOType);
    UNIT_TEST_SUITE_END();

public:
    TCoderTest() {
        NSignUrl::InitKey(Key, "123");
    }

    void EncodeCommonTest() {
        NSc::TValue music;
        music["Madonna"].SetString("Power_of_goodbye");
        music["Sting"].SetString("Shape_of_my_heart");

        NUgc::NCoder::Encode(music, Key);

        TString res = "{\"encoded\":\"T74_yiEE51PxvoZa5j4TUFNWeRXdwQHm28MxUhLd_dBbqzwfG2gC51ATXcQ3E19erobJrWodv8G0VUFlvP1l6g,,\"}";
        UNIT_ASSERT_VALUES_EQUAL(music.ToJson(), res);
        UNIT_ASSERT(music.Has("encoded"));
        UNIT_ASSERT(!music.Has("Madonna"));
        UNIT_ASSERT(!music.Has("Sting"));
    }

    void DecodeCommonTest() {
        NSc::TValue music;
        music["encoded"].SetString("T74_yiEE51PxvoZa5j4TUFNWeRXdwQHm28MxUhLd_dBbqzwfG2gC51ATXcQ3E19erobJrWodv8G0VUFlvP1l6g,,");

        NUgc::NCoder::Decode(music, Key);

        UNIT_ASSERT(!music.Has("encoded"));
        UNIT_ASSERT(music.Has("Madonna"));
        UNIT_ASSERT(music.Has("Sting"));

        UNIT_ASSERT_VALUES_EQUAL(music["Madonna"], "Power_of_goodbye");
        UNIT_ASSERT_VALUES_EQUAL(music["Sting"], "Shape_of_my_heart");
    }

    void EncodeArrayTest() {
        TString json = "[\"a\", \"b\"]";
        NSc::TValue arrValue = NSc::TValue::FromJson(json);

        UNIT_ASSERT(arrValue.IsArray());
        UNIT_ASSERT(!arrValue.IsDict());
        UNIT_ASSERT_EXCEPTION(NUgc::NCoder::Encode(arrValue, Key), yexception);
        UNIT_ASSERT_EXCEPTION(NUgc::NCoder::Decode(arrValue, Key), yexception);
    }

    void EncodeInnerArrayTest() {
        TString json = "{\"param\":[\"a\",\"b\"]}";
        NSc::TValue value = NSc::TValue::FromJson(json);

        UNIT_ASSERT(!value.IsArray());
        UNIT_ASSERT(value.IsDict());

        UNIT_ASSERT(value.Has("param"));
        UNIT_ASSERT(value["param"].IsArray());

        NUgc::NCoder::Encode(value, Key);
        UNIT_ASSERT(value.Has("encoded"));
        NUgc::NCoder::Decode(value, Key);
        UNIT_ASSERT_VALUES_EQUAL(json, value.ToJson());
    }

    void EncodeAlreadyEncodedTest() {
        NSc::TValue value;
        value["name"].SetString("value");
        value["encoded"].SetString("some_previously_encoded_info");

        NUgc::NCoder::Encode(value, Key);
        UNIT_ASSERT_VALUES_EQUAL(value["encoded"], "yXMFB7lFuZ1S1r2TzUibfA,,");
    }

    void DecodeAbsentFieldTest() {
        NSc::TValue value;
        UNIT_ASSERT_EXCEPTION(NUgc::NCoder::Decode(value, Key), yexception);
    }

    void EmptyKeyTest() {
        NSignUrl::TKey empKey;

        NSc::TValue value;
        value["name"].SetString("value");
        NUgc::NCoder::Encode(value, empKey);
        NUgc::NCoder::Decode(value, empKey);

        UNIT_ASSERT(value.Has("name"));
        UNIT_ASSERT_VALUES_EQUAL(value["name"].GetString(), "value");
    }

    void EncodeDecodeSomeInnerArrays() {
        TString source = "{\"params\":[{\"a\":\"b\"},{\"c\":[{\"aa\":\"bb\"},{\"cc\":\"dd\"}]}]}";
        NSc::TValue value = NSc::TValue::FromJson(source);
        NUgc::NCoder::Encode(value, Key);
        NUgc::NCoder::Decode(value, Key);

        UNIT_ASSERT_VALUES_EQUAL(source, value.ToJson());
    }

    void SizeTest() {
        NSc::TValue value;
        value["params"].SetString(TString(NUgc::NCoder::MAXSTRLEN + 1, '*'));

        UNIT_ASSERT_EXCEPTION(NUgc::NCoder::Encode(value, Key), yexception);
        UNIT_ASSERT_EXCEPTION(NUgc::NCoder::Decode(value, Key), yexception);
    }

    void CheckTokenInfo() {
        NSc::TValue userData = NSc::TValue::FromJsonThrow(R"({
            "errorCode":200,
            "type":"user-data",
            "updates":[{"value":"{\"objects\":[{
                                      \"notInterested\":\"no\",
                                      \"ratingOverall\":\"5\",
                                      \"key\":\"ruw123\"}],
                                  \"time\":1490677945000,
                                  \"type\":\"ugcupdate\",
                                  \"updateId\":\"serp/sas1-4637/ugc-853240752382-1\",
                                  \"userId\":\"/user/30273308\",
                                  \"version\":\"1.0\"}"}]
        })");
        NUgc::TFeedbackReader feedback;
        NUgc::TFeedbackReader::TErrorDescr error;
        feedback.LoadFromUserData(userData, error);

        NUgc::NCoder::TRatingInfoParams pr(Key, feedback);
        pr.AppId = "serp";
        pr.UserId = "/user/123";
        pr.VisitorId = "/visitor/456";
        pr.ContextId = "context-789";
        pr.ObjectKey = "ruw123";
        pr.Otype = "Film/Film";

        NSc::TValue ratingInfo = NUgc::NCoder::GetRatingInfo(pr);
        UNIT_ASSERT_VALUES_EQUAL(ratingInfo["rating_overall"], "5");
        UNIT_ASSERT_STRINGS_EQUAL(ratingInfo["not_interested"], "no");
        UNIT_ASSERT_VALUES_EQUAL(ratingInfo["rating_overall_10"], "10");

        for (const TStringBuf tokenField: {"token", "rating_overall_token", "not_interested_token", "rating_overall_10_token"}) {
            NSc::TValue res;
            res["encoded"] = ratingInfo[tokenField];

            NUgc::NCoder::Decode(res, Key);
            UNIT_ASSERT_VALUES_EQUAL(res["contextId"], "context-789");
            for (size_t i = 0; i < res["set"].GetArray().size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(res["set"][i]["key"], "ruw123");
            }
        }
    }

    void CheckTokenInfoFromSchemaObjects() {
        NUgc::NSchema::NOnto::TOntoObject object;
        object.SetOntoId("ruw123");
        object.MutableRatingOverall()->set_value(10);
        object.MutableNotInterested()->set_value(false);

        THashMap<TString, NUgc::NSchema::NOnto::TOntoObject> objects;
        objects.emplace("ruw123", std::move(object));

        NUgc::NCoder::TRatingInfoParams pr(Key, objects);
        pr.AppId = "serp";
        pr.UserId = "/user/123";
        pr.VisitorId = "/visitor/456";
        pr.ContextId = "context-789";
        pr.ObjectKey = "ruw123";
        pr.Otype = "Film/Film";

        NSc::TValue ratingInfo = NUgc::NCoder::GetRatingInfo(pr);
        UNIT_ASSERT_VALUES_EQUAL(ratingInfo["rating_overall"], "5");
        UNIT_ASSERT_STRINGS_EQUAL(ratingInfo["not_interested"], "no");
        UNIT_ASSERT_VALUES_EQUAL(ratingInfo["rating_overall_10"], "10");

        for (const TStringBuf tokenField: {"token", "rating_overall_token", "not_interested_token", "rating_overall_10_token"}) {
            NSc::TValue res;
            res["encoded"] = ratingInfo[tokenField];

            NUgc::NCoder::Decode(res, Key);
            UNIT_ASSERT_VALUES_EQUAL(res["contextId"], "context-789");
            for (size_t i = 0; i < res["set"].GetArray().size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(res["set"][i]["key"], "ruw123");
            }
        }
    }

    void CheckTokenEmptyProfile() {
        NSc::TValue userData = NSc::TValue::FromJsonThrow(R"({
            "errorCode":200,
            "type":"user-data",
            "status": "ok"
        })");
        NUgc::TFeedbackReader feedback;
        NUgc::TFeedbackReader::TErrorDescr error;
        feedback.LoadFromUserData(userData, error);

        NUgc::NCoder::TRatingInfoParams pr(Key, feedback);
        pr.AppId = "serp";
        pr.UserId = "/user/123";
        pr.VisitorId = "/visitor/456";
        pr.ContextId = "context-789";
        pr.ObjectKey = "ruw123";
        pr.Otype = "Film/Film";

        NSc::TValue ratingInfo = NUgc::NCoder::GetRatingInfo(pr);
        UNIT_ASSERT(!ratingInfo.Has("rating_overall"));
        UNIT_ASSERT(!ratingInfo.Has("not_interested"));
        UNIT_ASSERT(!ratingInfo.Has("rating_overall_10"));

        for (const TStringBuf tokenField: {"token", "rating_overall_token", "not_interested_token", "rating_overall_10_token"}) {
            NSc::TValue res;
            res["encoded"] = ratingInfo[tokenField];

            NUgc::NCoder::Decode(res, Key);
            UNIT_ASSERT_VALUES_EQUAL(res["contextId"], "context-789");
            UNIT_ASSERT_VALUES_EQUAL(res["set"][0]["key"], "ruw123");
        }
    }

    void CheckTokenEmptyProfileFromSchemaObjects() {
        THashMap<TString, NUgc::NSchema::NOnto::TOntoObject> objects;

        NUgc::NCoder::TRatingInfoParams pr(Key, objects);
        pr.AppId = "serp";
        pr.UserId = "/user/123";
        pr.VisitorId = "/visitor/456";
        pr.ContextId = "context-789";
        pr.ObjectKey = "ruw123";
        pr.Otype = "Film/Film";

        NSc::TValue ratingInfo = NUgc::NCoder::GetRatingInfo(pr);
        UNIT_ASSERT(!ratingInfo.Has("rating_overall"));
        UNIT_ASSERT(!ratingInfo.Has("not_interested"));
        UNIT_ASSERT(!ratingInfo.Has("rating_overall_10"));

        for (const TStringBuf tokenField: {"token", "rating_overall_token", "not_interested_token", "rating_overall_10_token"}) {
            NSc::TValue res;
            res["encoded"] = ratingInfo[tokenField];

            NUgc::NCoder::Decode(res, Key);
            UNIT_ASSERT_VALUES_EQUAL(res["contextId"], "context-789");
            UNIT_ASSERT_VALUES_EQUAL(res["set"][0]["key"], "ruw123");
            UNIT_ASSERT_VALUES_EQUAL(res["set"][1]["key"], "ruw123");
            UNIT_ASSERT_VALUES_EQUAL(res["set"][1]["value"], "Film/Film");
        }
    }

    void CheckOType() {
        NSc::TValue userData = NSc::TValue::FromJsonThrow(R"({
            "errorCode":200,
            "type":"user-data",
            "status": "ok"
        })");
        NUgc::TFeedbackReader feedback;
        NUgc::TFeedbackReader::TErrorDescr error;
        feedback.LoadFromUserData(userData, error);

        NUgc::NCoder::TRatingInfoParams pr(Key, feedback);
        pr.AppId = "serp";
        pr.UserId = "/user/123";
        pr.VisitorId = "/visitor/456";
        pr.ContextId = "context-789";
        pr.ObjectKey = "ruw123";
        pr.Otype = "Film/Film";

        NSc::TValue info = NUgc::NCoder::GetRatingInfo(pr);
        for (const TStringBuf tokenField: {"token", "rating_overall_token", "not_interested_token", "rating_overall_10_token"}) {
            NSc::TValue res;
            res["encoded"] = info[tokenField];

            NUgc::NCoder::Decode(res, Key);
            UNIT_ASSERT_VALUES_EQUAL(res["set"][0]["key"], "ruw123");
            UNIT_ASSERT_VALUES_EQUAL(res["set"][1]["key"], "ruw123");
            UNIT_ASSERT_VALUES_EQUAL(res["set"][1]["value"], "Film/Film");
        }
    }

    void CheckNoOType() {
        NSc::TValue userData = NSc::TValue::FromJsonThrow(R"({
            "errorCode":200,
            "type":"user-data",
            "status": "ok"
        })");
        NUgc::TFeedbackReader feedback;
        NUgc::TFeedbackReader::TErrorDescr error;
        feedback.LoadFromUserData(userData, error);

        NUgc::NCoder::TRatingInfoParams pr(Key, feedback);
        pr.AppId = "serp";
        pr.UserId = "/user/123";
        pr.VisitorId = "/visitor/456";
        pr.ContextId = "context-789";
        pr.ObjectKey = "ruw123";

        NSc::TValue info = NUgc::NCoder::GetRatingInfo(pr);
        UNIT_ASSERT_VALUES_EQUAL(info, NSc::TValue());
    }

private:
    NSignUrl::TKey Key;
};

UNIT_TEST_SUITE_REGISTRATION(TCoderTest);
