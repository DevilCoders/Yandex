#include <kernel/ugc/aggregation/feedback_v2.h>

#include <library/cpp/json/writer/json.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <google/protobuf/util/json_util.h>
#include <util/system/backtrace.h>

namespace {

    class TRes {
    public:
        TRes(TStringBuf path)
                : Path_(path) {
        }

        const NJson::TJsonValue& Data() const {
            if (!Data_.IsDefined()) {
                const TString data = NResource::Find(Path_);

                if (!data) {
                    ythrow yexception() << "Unable to load resource " << Path_;
                }
                NJson::ReadJsonFastTree(data, &Data_);
            }

            return Data_;
        }

        operator const NJson::TJsonValue&() const {
            return Data();
        }

    private:
        const TStringBuf Path_;
        mutable NJson::TJsonValue Data_;
    };

    void UNIT_ASSERT_CONTEXT_EQUALS(const NUgc::TFeedbackAggregator& fb, const NJson::TJsonValue& expected) {
        TString jsonStr;
        ::google::protobuf::util::MessageToJsonString(fb.GetContext(), &jsonStr);

        NJson::TJsonValue contextJson = NJson::ReadJsonFastTree(jsonStr);

        UNIT_ASSERT_EQUAL(contextJson, expected);
    }

    TVector<TString> NormalizePhotos(const NUgc::TFeedbackAggregator::TPhotosList& photos) {
        return TVector<TString>(photos.begin(), photos.end());
    }

    static const TRes DATA1_RESULT("/data1/result_v2");
    static const TRes DATA1_UPDATES("/data1/updates");
    static const TString TestFeedbackId = "mIGcoMetdlOQcvkbIOo1l-yChLVivLFjh";

    NJson::TJsonValue MakeObjectUpdate(ui64 time, const TString& key, const TString& col, const TString& text) {
        NJson::TJsonValue update;
        update["userId"] = "/user/123";
        update["version"] = "1.0";
        update["app"] = "serp";
        update["type"] = "ugcupdate";
        update["time"] = time;

        NJson::TJsonValue object;
        object["key"] = key;
        object[col] = text;

        update["objects"].AppendValue(object);
        return update;
    }
}

Y_UNIT_TEST_SUITE(TFeedbackV2Test) {
    Y_UNIT_TEST(AppendFeedbacksTest) {
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;

        aggregator.AppendFeedbacks(DATA1_UPDATES.Data().GetArray(), error);

        UNIT_ASSERT(aggregator.AppendFeedbacks(DATA1_UPDATES.Data().GetArray(), error));
        UNIT_ASSERT(!error);

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, DATA1_RESULT.Data()[0]);
    }

    Y_UNIT_TEST(LoadFromUserDataTest) {
        NJson::TJsonValue userdata;
        userdata["type"].SetValue("user-data");
        for (const NJson::TJsonValue& update : DATA1_UPDATES.Data().GetArray()) {
            NJson::TJsonValue newUpdate;
            newUpdate["value"].SetValue(NJson::WriteJson(update));
            newUpdate["app"] = update["app"];
            newUpdate["time"] = update["time"];
            userdata["updates"].AppendValue(newUpdate);
        }

        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT_VALUES_EQUAL(aggregator.LoadFromUserData(userdata, error), userdata["updates"].GetArray().size());
        UNIT_ASSERT(!error);

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, DATA1_RESULT.Data()[0]);

        const auto* props = aggregator.GetObjectProps("objects", "o1");
        UNIT_ASSERT(props);
        const auto like = props->find("iLikeIt");
        UNIT_ASSERT_STRINGS_EQUAL(like->second.GetValue(), "true");

        const auto text = props->find("opinionText");
        UNIT_ASSERT_STRINGS_EQUAL(text->second.GetValue(), "good");

        UNIT_ASSERT_STRINGS_EQUAL(*aggregator.Value("objects", "o1", "opinionText"), "good");
    }

    Y_UNIT_TEST(TombstoneDirectOrderTest) {
        const TRes result("/data2/direct_order_result_v2");

        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data2/direct_order").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(TombstoneReverseOrderTest) {
        const TRes result("/data2/reverse_order_result_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data2/reverse_order").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(CommonTest) {
        NUgc::TFeedbackAggregator aggregator;

        NJson::TJsonValue update;
        update.AppendValue(DATA1_UPDATES.Data()[0]);

        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(update.GetArray(), error));
        UNIT_ASSERT(!error);

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, DATA1_RESULT.Data()[1]);
//
//        {
//            NUgc::TFeedbackAggregator::TErrorDescr tmpError;
//
//            UNIT_ASSERT(!aggregator.AppendFeedbacks(NJson::TJsonValue::TArray(), tmpError));
//            UNIT_ASSERT(tmpError);
//        }
    }
//
    Y_UNIT_TEST(CastValueToStringTest) {
        const TRes result("/data4/result_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data4/updates").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(SkipInvalidValuesTest) {
        const TRes result("/data5/result_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data5/updates").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(BuildReviewFeedbackIdTest) {
        const TRes result("/data6/result_feedback_review_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data6/updates_feedback_review").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(BuildRatingFeedbackIdTest) {
        const TRes result("/data6/result_feedback_rating_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data6/updates_feedback_rating").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(OverrideBuildReviewFeedbackIdTest) {
        const TRes result("/data6/result_feedback_override_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data6/updates_feedback_override").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(CustomTableKeyNameTest) {
        const TRes result("/data7/result_v2");
        NUgc::TFeedbackAggregator aggregator("parentId");
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data7/updates").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(SkippedReviewTest) {
        const TRes result("/data8/result_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data8/updates").Data().GetArray(), error,
                                       NUgc::TFeedbackAggregator::TAppendOptions().FilterSkippedReviews(true)));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(ValidReviewAfterSkippedReviewTest) {
        const TRes result("/data9/result_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data9/updates").Data().GetArray(), error,
                                       NUgc::TFeedbackAggregator::TAppendOptions().FilterSkippedReviews(true)));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(InvalidRatingsTest) {
        const TRes result("/data10/result_v2");
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        UNIT_ASSERT(aggregator.AppendFeedbacks(TRes("/data10/updates").Data().GetArray(), error));

        UNIT_ASSERT_CONTEXT_EQUALS(aggregator, result.Data());
    }

    Y_UNIT_TEST(PatchFeedbackIdForRatingReviewTest) {
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        NJson::TJsonValue updates;
        updates.AppendValue(MakeObjectUpdate(123456789010ul, "/sprav/1001", "ratingOverall",
                                             "5")); // generate feedbackId for empty review
        updates.AppendValue(
                MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test")); // generate feedbackId
        updates.AppendValue(
                MakeObjectUpdate(123456789030ul, "/sprav/1001", "ratingOverall", "4")); // keep feedbackId

        UNIT_ASSERT(aggregator.AppendFeedbacks(updates.GetArray(), error));

        // feedbackId is same with PatchFeedbackIdForReviewTest
        UNIT_ASSERT_STRINGS_EQUAL(*aggregator.Value("objects", "/sprav/1001", "feedbackId"), TestFeedbackId);
    }

    Y_UNIT_TEST(PatchFeedbackIdForReviewTest) {
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        NJson::TJsonValue updates;
        updates.AppendValue(MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test"));

        UNIT_ASSERT(aggregator.AppendFeedbacks(updates.GetArray(), error));

        // review feedbackId for empty review
        UNIT_ASSERT_STRINGS_EQUAL(*aggregator.Value("objects", "/sprav/1001", "feedbackId"), TestFeedbackId);
    }

    Y_UNIT_TEST(PatchFeedbackIdForRatingTest) {
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        NJson::TJsonValue updates;
        updates.AppendValue(MakeObjectUpdate(123456789010ul, "/sprav/1001", "ratingOverall", "5"));

        UNIT_ASSERT(aggregator.AppendFeedbacks(updates.GetArray(), error));

        // rating feedbackId based on empty review
        UNIT_ASSERT_STRINGS_EQUAL(*aggregator.Value("objects", "/sprav/1001", "feedbackId"),
                                  "hT_DyVCipt8Va8PMZCY0_ZuXFGPLlZcz");
    }

    Y_UNIT_TEST(ReviewTimeTest) {
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;
        NJson::TJsonValue updates;

        updates.AppendValue(MakeObjectUpdate(123456789030ul, "/sprav/1001", "reviewOverall", "test review"));
        updates.AppendValue(
                MakeObjectUpdate(123456789010ul, "/sprav/1001", "reviewOverall", "test review")); // expected
        updates.AppendValue(MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test review"));

        updates.AppendValue(MakeObjectUpdate(123456789110ul, "/sprav/1003", "reviewOverall", "test review old"));
        updates.AppendValue(MakeObjectUpdate(123456789120ul, "/sprav/1003", "reviewOverall", "test review new"));
        updates.AppendValue(
                MakeObjectUpdate(123456789130ul, "/sprav/1003", "reviewOverall", "test review old")); // expected
        updates.AppendValue(MakeObjectUpdate(123456789140ul, "/sprav/1003", "reviewOverall", "test review old"));

        updates.AppendValue(
                MakeObjectUpdate(123456789210ul, "/sprav/1005", "reviewOverall", "test review 1005")); // expected

        auto options = NUgc::TFeedbackAggregator::TAppendOptions().AddReviewsTime(true);
        UNIT_ASSERT(aggregator.AppendFeedbacks(updates.GetArray(), error, options));
        UNIT_ASSERT_EQUAL(aggregator.GetObjectReviewTime("/sprav/1001"), TInstant::MicroSeconds(123456789010000ul));
        UNIT_ASSERT_EQUAL(aggregator.GetObjectReviewTime("/sprav/1002"), TInstant::Zero());
        UNIT_ASSERT_EQUAL(aggregator.GetObjectReviewTime("/sprav/1003"), TInstant::MicroSeconds(123456789130000ul));
        UNIT_ASSERT_EQUAL(aggregator.GetObjectReviewTime("/sprav/1005"), TInstant::MicroSeconds(123456789210000ul));
    }

    Y_UNIT_TEST(AddPhotosTest) {
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;

        NJson::TJsonValue updates;
        updates.AppendValue(MakeObjectUpdate(123456789010ul, "/sprav/1001", "photoId", "1"));
        updates.AppendValue(MakeObjectUpdate(123456789020ul, "/sprav/1001", "photoId", "2"));

        auto options = NUgc::TFeedbackAggregator::TAppendOptions().AddPhotos(true);
        UNIT_ASSERT(aggregator.AppendFeedbacks(updates.GetArray(), error, options));

        UNIT_ASSERT_EQUAL(NormalizePhotos(aggregator.GetObjectPhotos("/sprav/1001")), (TVector<TString>{"1", "2"}));
        UNIT_ASSERT_EQUAL(NormalizePhotos(aggregator.GetObjectPhotos("/sprav/1002")), TVector<TString>{});
    }

    Y_UNIT_TEST(DoubleWasHere) {
        NUgc::TFeedbackAggregator aggregator;
        NUgc::TFeedbackAggregator::TErrorDescr error;

        NJson::TJsonValue updates;
        updates.AppendValue(MakeObjectUpdate(123456789010ul, "/sprav/1", "wasHere", "false"));
        updates.AppendValue(MakeObjectUpdate(123456789020ul, "/sprav/1", "wasHere", "false"));
        auto options = NUgc::TFeedbackAggregator::TAppendOptions();
        UNIT_ASSERT(aggregator.AppendFeedbacks(updates.GetArray(), error, options));

        const auto* props = aggregator.GetObjectProps("objects", "/sprav/1");
        UNIT_ASSERT(props);
        const auto washere = props->find("wasHere");
        const ui64 expected = 123456789020;

        UNIT_ASSERT_EQUAL(washere->second.GetTime(), expected);
    }
}
