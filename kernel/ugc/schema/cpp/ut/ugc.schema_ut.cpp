#include <kernel/ugc/schema/proto/ugc.pb.h>
#include <kernel/ugc/schema/cpp/ugc.schema.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/protobuf/json/json2proto.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NUgc {
    Y_UNIT_TEST_SUITE(TCreateUgcEntityTest) {
        Y_UNIT_TEST(CreatesEntityFromPath) {
            TUgcEntity entity = CreateUgcEntity("/user/123/sprav/456/feedback/abc");
            UNIT_ASSERT_EQUAL(entity.GetRowCase(), TUgcEntity::kUserFeedback);

            google::protobuf::Message* row = GetEntityRow(entity);
            UNIT_ASSERT(dynamic_cast<TUserFeedback*>(row));

            TUserFeedback fb;
            fb.CopyFrom(*row);

            UNIT_ASSERT_STRINGS_EQUAL(fb.GetUserId(), "/user/123");
            UNIT_ASSERT_STRINGS_EQUAL(fb.GetObjectId(), "/sprav/456");
            UNIT_ASSERT_STRINGS_EQUAL(fb.GetFeedbackId(), "abc");
        }
    }

    Y_UNIT_TEST_SUITE(TUpdateProtoTest) {
        Y_UNIT_TEST(ParsesFromJson) {
            TString s =
                "{\"app\":\"maps\",\"objects\":[{"
                "\"appType\":\"maps-desktop\",\"feedbackId\":\"abc123\","
                "\"key\":\"/sprav/123\",\"ratingOverall\":\"4.5\",\"reviewOverall\":\"Cool!\",\"signPrivacy\":\"NAME\"}],"
                "\"time\":1234567890,\"type\":\"ugcupdate\",\"updateId\":\"update/1\","
                "\"userId\":\"/user/456\",\"version\":\"1.0\",\"visitorId\":\"/visitor/789\"}";

            TUserUpdateProto update = JsonToUserUpdateProto(s);

            UNIT_ASSERT_STRINGS_EQUAL(update.GetApp(), "maps");
            UNIT_ASSERT_EQUAL(update.GetTime(), 1234567890u);
            UNIT_ASSERT_STRINGS_EQUAL(update.GetType(), "ugcupdate");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUpdateId(), "update/1");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserId(), "/user/456");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetVersion(), "1.0");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetVisitorId(), "/visitor/789");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserObject(0).GetAppType(), "maps-desktop");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserObject(0).GetFeedbackId(), "abc123");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserObject(0).GetKey(), "/sprav/123");
            UNIT_ASSERT_EQUAL(update.GetUserObject(0).GetRatingOverall(), 4.5);
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserObject(0).GetReviewOverall(), "Cool!");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserObject(0).GetSignPrivacy(), "NAME");
        }

        Y_UNIT_TEST(CreatesUpdateFromRow) {
            TUserFeedback row;
            row.SetUserId("/user/123");
            row.SetObjectId("/sprav/456");
            row.SetRatingOverall(4.5);
            row.SetReviewOverall("Cool!");
            row.SetHidden(true);

            TUserUpdateProto update = CreateUserUpdateProto(row);

            UNIT_ASSERT_STRINGS_EQUAL(update.GetType(), "ugcupdate");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetVersion(), "1.0");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserId(), "/user/123");
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserObject(0).GetKey(), "/sprav/456");
            UNIT_ASSERT_EQUAL(update.GetUserObject(0).GetRatingOverall(), 4.5);
            UNIT_ASSERT_STRINGS_EQUAL(update.GetUserObject(0).GetReviewOverall(), "Cool!");
            UNIT_ASSERT_EQUAL(update.GetUserObject(0).GetHidden(), true);
        }
    }
}
