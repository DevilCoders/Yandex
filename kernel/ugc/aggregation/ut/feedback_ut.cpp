#include <kernel/ugc/aggregation/feedback.h>
#include <kernel/ugc/aggregation/proto/review_meta.pb.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

namespace {
    class TRes {
    public:
        TRes(TStringBuf path)
                : Path_(path)
        {
        }

        const NSc::TValue& Data() const {
            if (Data_.IsNull()) {
                const TString data = NResource::Find(Path_);
                if (!data) {
                    ythrow yexception() << "Unable to load resource " << Path_;
                }
                Data_ = NSc::TValue::FromJsonThrow(data);
            }

            return Data_;
        }

        operator const NSc::TValue&() const {
            return Data();
        }

    private:
        const TStringBuf Path_;
        mutable NSc::TValue Data_;
    };

    static const TRes DATA1_RESULT("/data1/result");
    static const TRes DATA1_UPDATES("/data1/updates");
    static const TString TestFeedbackId = "mIGcoMetdlOQcvkbIOo1l-yChLVivLFjh";

}

Y_UNIT_TEST_SUITE(TFeedbackTest) {
    Y_UNIT_TEST(AppendFeedbacksTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(DATA1_UPDATES.Data().GetArray(), error));
        UNIT_ASSERT(!error);
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(false), DATA1_RESULT.Data()[0]);
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(true), DATA1_RESULT.Data()[1]);
    }

    Y_UNIT_TEST(LoadFromUserDataTest) {
        NSc::TValue userdata;
        userdata["type"].SetString("user-data");
        for (const NSc::TValue& update : DATA1_UPDATES.Data().GetArray()) {
            NSc::TValue newUpdate;
            newUpdate["value"].SetString(update.ToJson());
            newUpdate["app"] = update["app"];
            newUpdate["time"] = update["time"];
            userdata["updates"].GetArrayMutable().push_front(newUpdate);
        }

        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT_VALUES_EQUAL(fr.LoadFromUserData(userdata, error), userdata["updates"].ArraySize());
        UNIT_ASSERT(!error);
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(false), DATA1_RESULT.Data()[0]);
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(true), DATA1_RESULT.Data()[1]);

        const auto* props = fr.GetObjectProps("objects", "o1");
        UNIT_ASSERT(props);
        const auto like = props->find("iLikeIt");
        UNIT_ASSERT_STRINGS_EQUAL(like->second.Value, "true");

        const auto text = props->find("opinionText");
        UNIT_ASSERT_STRINGS_EQUAL(text->second.Value, "good");

        UNIT_ASSERT_STRINGS_EQUAL(*fr.Value("objects", "o1", "opinionText"), "good");
    }

    Y_UNIT_TEST(TombstoneDirectOrderTest) {
        const TRes result("/data2/direct_order_result");

        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data2/direct_order").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data()[0]);
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(true), result.Data()[1]);
    }

    Y_UNIT_TEST(TombstoneReverseOrderTest) {
        const TRes result("/data2/reverse_order_result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data2/reverse_order").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(true), result.Data());
    }

    Y_UNIT_TEST(TombstoneFeedbackIdRatingOrderTest) {
        const TRes result("/data2/order_test1_result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data2/order_test1").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(true), result.Data());
    }

    Y_UNIT_TEST(TombstoneNoFeedbackIdRatingOrderTest) {
        const TRes result("/data2/order_test2_result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data2/order_test2").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(true), result.Data());
    }

    Y_UNIT_TEST(SortFeedbackTest) {
        const TRes result("/data3/result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data3/updates").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(true, true), result.Data());
    }

    Y_UNIT_TEST(UpdateIdBuilder) {
        NUgc::TFeedbackReader fr;

        // TODO add more values to check
        class TSimpleUpdateIdBuilder: public NUgc::TFeedbackReader::IUpdateIdBuilder {
        public:
            void OnFeedback(const NSc::TValue& feedback) override {
                const time_t updateTime = feedback[TStringBuf("time")];

                if (!Min_ || Min_ > updateTime) {
                    Min_ = updateTime;
                }

                if (!Max_ || Max_ < updateTime) {
                    Max_ = updateTime;
                }
            }

            TString Result() override {
                return TStringBuilder() << "compacted-" << Min_ << "-" << Max_;
            }

        private:
            time_t Min_ = 0;
            time_t Max_ = 0;
        };

        fr.SetUpdateIdBuilder(MakeHolder<TSimpleUpdateIdBuilder>());

        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(DATA1_UPDATES.Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL("compacted-1234567890-1234567895", fr.Aggregate()["updateId"].GetString());
    }

    Y_UNIT_TEST(CommonTest) {
        NUgc::TFeedbackReader fr;
        UNIT_ASSERT(fr.Aggregate().IsNull());

        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedback(DATA1_UPDATES.Data()[0], error));
        UNIT_ASSERT(!error);
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), DATA1_RESULT.Data()[2]);

        {
            NUgc::TFeedbackReader::TErrorDescr tmpError;

            UNIT_ASSERT(!fr.AppendFeedback(NSc::TValue(), tmpError));
            UNIT_ASSERT(tmpError);
        }
    }

    Y_UNIT_TEST(CastValueToStringTest) {
        const TRes result("/data4/result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data4/updates").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(SkipInvalidValuesTest) {
        const TRes result("/data5/result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data5/updates").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(BuildReviewFeedbackIdTest) {
        const TRes result("/data6/result_feedback_review");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data6/updates_feedback_review").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(BuildRatingFeedbackIdTest) {
        const TRes result("/data6/result_feedback_rating");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data6/updates_feedback_rating").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(OverrideBuildReviewFeedbackIdTest) {
        const TRes result("/data6/result_feedback_override");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data6/updates_feedback_override").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(CustomTableKeyNameTest) {
        const TRes result("/data7/result");
        NUgc::TFeedbackReader fr("parentId");
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data7/updates").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(SkippedReviewTest) {
        const TRes result("/data8/result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data8/updates").Data().GetArray(), error, NUgc::TFeedbackReader::TAppendOptions().FilterSkippedReviews(true)));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(ValidReviewAfterSkippedReviewTest) {
        const TRes result("/data9/result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data9/updates").Data().GetArray(), error, NUgc::TFeedbackReader::TAppendOptions().FilterSkippedReviews(true)));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    Y_UNIT_TEST(InvalidRatingsTest) {
        const TRes result("/data10/result");
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        UNIT_ASSERT(fr.AppendFeedbacks(TRes("/data10/updates").Data().GetArray(), error));
        UNIT_ASSERT_VALUES_EQUAL(fr.Aggregate(), result.Data());
    }

    NSc::TValue MakeObjectUpdate(ui64 time, const TString& key, const TString& col, const TString& text) {
        NSc::TValue update;
        update["userId"] = "/user/123";
        update["version"] = "1.0";
        update["app"] = "serp";
        update["type"] = "ugcupdate";
        update["time"] = time;
        auto& object = update["objects"].Push();
        object["key"] = key;
        object[col] = text;
        return update;
    }

    NSc::TValue MakeObjectUpdate(
            ui64 time,
            const TString& key,
            const TString& col,
            const TString& text,
            const TVector<std::pair<TString, TString>>& updateColumns)
    {
        NSc::TValue update;
        update["userId"] = "/user/123";
        update["version"] = "1.0";
        update["app"] = "serp";
        update["type"] = "ugcupdate";
        update["time"] = time;
        auto& object = update["objects"].Push();
        object["key"] = key;
        object[col] = text;
        for (const auto& column : updateColumns) {
            update[column.first] = column.second;
        }
        return update;
    }

    Y_UNIT_TEST(FeedbackIdByTimestamp) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;

        auto upd1 = MakeObjectUpdate(123456789010ul, "/sprav/1001", "reviewOverall", "some review");
        upd1["objects"][0]["feedbackId"] = "dummyFeedbackId";

        auto upd2 = MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test"); // generate feedbackId

        updates.AppendAll({upd1, upd2});

        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error, NUgc::NFeedbackReaderUtils::TAppendOptions().AddFeedbackIds(true)));

        UNIT_ASSERT_STRINGS_EQUAL(*fr.GetFeedbackIdByTimestamp("/sprav/1001", TInstant::MilliSeconds(123456789010ul)), "dummyFeedbackId");

        // feedbackId is same with PatchFeedbackIdForReviewTest
        UNIT_ASSERT_STRINGS_EQUAL(*fr.GetFeedbackIdByTimestamp("/sprav/1001", TInstant::MilliSeconds(123456789020ul)), TestFeedbackId);
    }

    Y_UNIT_TEST(PatchFeedbackIdForRatingReviewTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789010ul, "/sprav/1001", "ratingOverall", "5"),    // generate feedbackId for empty review
            MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test"), // generate feedbackId
            MakeObjectUpdate(123456789030ul, "/sprav/1001", "ratingOverall", "4"),    // keep feedbackId
        });

        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error));

        // feedbackId is same with PatchFeedbackIdForReviewTest
        UNIT_ASSERT_STRINGS_EQUAL(*fr.Value("objects", "/sprav/1001", "feedbackId"), TestFeedbackId);
    }

    Y_UNIT_TEST(PatchFeedbackIdForReviewTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test"),
        });

        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error));

        // review feedbackId for empty review
        UNIT_ASSERT_STRINGS_EQUAL(*fr.Value("objects", "/sprav/1001", "feedbackId"), TestFeedbackId);
    }

    Y_UNIT_TEST(PatchFeedbackIdForRatingTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789010ul, "/sprav/1001", "ratingOverall", "5"),
        });

        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error));

        // rating feedbackId based on empty review
        UNIT_ASSERT_STRINGS_EQUAL(*fr.Value("objects", "/sprav/1001", "feedbackId"), "hT_DyVCipt8Va8PMZCY0_ZuXFGPLlZcz");
    }

    Y_UNIT_TEST(ReviewTimeTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789030ul, "/sprav/1001", "reviewOverall", "test review"),
            MakeObjectUpdate(123456789010ul, "/sprav/1001", "reviewOverall", "test review"), // expected
            MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test review"),

            MakeObjectUpdate(123456789110ul, "/sprav/1003", "reviewOverall", "test review old"),
            MakeObjectUpdate(123456789120ul, "/sprav/1003", "reviewOverall", "test review new"),
            MakeObjectUpdate(123456789130ul, "/sprav/1003", "reviewOverall", "test review old"), // expected
            MakeObjectUpdate(123456789140ul, "/sprav/1003", "reviewOverall", "test review old"),

            MakeObjectUpdate(123456789210ul, "/sprav/1005", "reviewOverall", "test review 1005"), // expected
        });
        auto options = NUgc::TFeedbackReader::TAppendOptions().AddReviewsTime(true);
        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error, options));
        UNIT_ASSERT_EQUAL(fr.GetObjectReviewTime("/sprav/1001"), TInstant::MicroSeconds(123456789010000ul));
        UNIT_ASSERT_EQUAL(fr.GetObjectReviewTime("/sprav/1002"), TInstant::Zero());
        UNIT_ASSERT_EQUAL(fr.GetObjectReviewTime("/sprav/1003"), TInstant::MicroSeconds(123456789130000ul));
        UNIT_ASSERT_EQUAL(fr.GetObjectReviewTime("/sprav/1005"), TInstant::MicroSeconds(123456789210000ul));
    }

    Y_UNIT_TEST(RatingTimeTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789030ul, "/sprav/1001", "ratingOverall", "5"),
            MakeObjectUpdate(123456789010ul, "/sprav/1001", "ratingOverall", "3"),
            MakeObjectUpdate(123456789020ul, "/sprav/1001", "ratingOverall", "4"),
        });
        auto options = NUgc::TFeedbackReader::TAppendOptions().AddReviewsTime(true);
        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error, options));
        UNIT_ASSERT_EQUAL(fr.GetObjectRatingTime("/sprav/1001"), TInstant::MilliSeconds(123456789030ul));
        UNIT_ASSERT_EQUAL(fr.GetObjectRatingTime("/sprav/1002"), TInstant::Zero());
    }

    Y_UNIT_TEST(AddMetaTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789000ul, "/sprav/1001", "reviewOverall", "test review"),
            MakeObjectUpdate(123456789010ul, "/sprav/1001", "reviewOverall", "test review", {{"yandexTld", "ru"}}),
            MakeObjectUpdate(123456789020ul, "/sprav/1001", "reviewOverall", "test review", {{"countryCode", "157"}}),
            MakeObjectUpdate(123456789040ul, "/sprav/1001", "reviewOverall", "test review updated", {
                {"yandexTld", "ru"},
                {"countryCode", "235"},
                {"updateId", "new_update_id"}
            }), // expected
            MakeObjectUpdate(123456789030ul, "/sprav/1001", "reviewOverall", "test review", {{"updateId", "old_update_id"}}),
        });
        auto options = NUgc::TFeedbackReader::TAppendOptions().AddMeta(true).AddReviewsTime(true);
        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error, options));

        NUgc::TReviewMeta reviewMeta = fr.GetObjectReviewMeta("/sprav/1001");
        UNIT_ASSERT_STRINGS_EQUAL(reviewMeta.GetYandexTLD(), "ru");
        UNIT_ASSERT_STRINGS_EQUAL(reviewMeta.GetCountryId(), "235");
        UNIT_ASSERT_STRINGS_EQUAL(reviewMeta.GetUpdateId(), "new_update_id");
    }

    Y_UNIT_TEST(AddPhotosTest) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789010ul, "/sprav/1001", "photoId", "1"),
            MakeObjectUpdate(123456789020ul, "/sprav/1001", "photoId", "2"),
        });
        auto options = NUgc::TFeedbackReader::TAppendOptions().AddPhotos(true);
        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error, options));
        UNIT_ASSERT_EQUAL(fr.GetObjectPhotos("/sprav/1001"), (TVector<TString>{"1", "2"}));
        UNIT_ASSERT_EQUAL(fr.GetObjectPhotos("/sprav/1002"), TVector<TString>{});
    }

    Y_UNIT_TEST(DoubleWasHere) {
        NUgc::TFeedbackReader fr;
        NUgc::TFeedbackReader::TErrorDescr error;
        NSc::TValue updates;
        updates.AppendAll({
            MakeObjectUpdate(123456789010ul, "/sprav/1", "wasHere", "false"),
            MakeObjectUpdate(123456789020ul, "/sprav/1", "wasHere", "false"),
        });
        auto options = NUgc::TFeedbackReader::TAppendOptions();
        UNIT_ASSERT(fr.AppendFeedbacks(updates.GetArray(), error, options));

        const auto* props = fr.GetObjectProps("objects", "/sprav/1");
        UNIT_ASSERT(props);
        const auto washere = props->find("wasHere");
        auto expected = TInstant::MilliSeconds(123456789020);

        UNIT_ASSERT_EQUAL(washere->second.Time, expected);
    }
}
