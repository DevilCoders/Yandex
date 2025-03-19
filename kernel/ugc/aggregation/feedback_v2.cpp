#include "feedback_v2.h"
#include "utils.h"

#include <kernel/ugc/security/lib/record_identifier.h>

#include <library/cpp/json/writer/json.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <util/datetime/base.h>
#include <util/string/builder.h>
#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>

namespace NUgc {
    namespace {
        void PatchFeedbackId(const TString& key, ui64 updateTime, const NJson::TJsonValue& feedback,
                             TFeedbackContext::TObjectProps& object) {
            // find old or current review text
            auto iter = object.GetProps().find("reviewOverall");
            TString review = iter == object.GetProps().end() ? "" : iter->second.GetValue();

            // build feedbackId on (userId, orgId, reviewText)
            auto& reviewFeedbackId = (*object.MutableProps())["feedbackId"];
            reviewFeedbackId.SetTime(updateTime);
            reviewFeedbackId.SetValue(
                    NFeedbackReaderUtils::BuildUserReviewFeedbackId(
                            TString(feedback["userId"].GetString()),
                            key,
                            review
                    )
            );
        }
    }

    TFeedbackAggregator::TFeedbackAggregator()
            : TFeedbackAggregator("key") {
    }

    TFeedbackAggregator::TFeedbackAggregator(TStringBuf tableKeyName) {
        Context_.SetTableKeyName(TString(tableKeyName));
    }

    TFeedbackAggregator::TFeedbackAggregator(const TFeedbackContext& context)
            : Context_(context) {
    }

    const TFeedbackAggregator::TObjectProps* TFeedbackAggregator::GetObjectProps(TStringBuf tableName, TStringBuf objectName) const {
        const auto& tables = Context_.GetData().GetTables();
        const auto table = tables.find(TString(tableName));
        if (table == tables.end()) {
            return nullptr;
        }

        const auto& objects = table->second.GetRows();
        const auto object = objects.find(TString(objectName));

        return object != objects.end() ? &(object->second.GetProps()) : nullptr;
    }

    TMaybe<TStringBuf> TFeedbackAggregator::Value(TStringBuf tableName, TStringBuf objectName, TStringBuf columnName) const {
        const TObjectProps* props = GetObjectProps(tableName, objectName);
        if (!props) {
            return Nothing();
        }

        const auto column = props->find(TString(columnName));
        if (column == props->end()) {
            return Nothing();
        }

        return TStringBuf(column->second.GetValue());
    }

    TInstant TFeedbackAggregator::GetObjectReviewTime(TStringBuf objectName) const {
        const auto& objects = Context_.GetReviewsData().GetObjects();
        const auto object = objects.find(TString(objectName));
        if (object == objects.end()) {
            return TInstant::Zero();
        }

        // only one review can be for each object
        // we need to find first Time of last Review duplicates
        // TMap has sorted reviews by time.
        // we step from the end (oldest) to the begin (newest) to find last review time

        // example updates (reviewTime, reviewText):
        // 1, text_new
        // 2, text_old
        // 3, text_new <- it is answer
        // 4, text_new

        ::google::protobuf::RepeatedPtrField<::NUgc::TFeedbackContext::TReviewInfo> reviews(
                object->second.GetReviews());

        ui64 curTime = 0;
        TString curReview;

        Sort(reviews,
             [](const TFeedbackContext::TReviewInfo& a, const TFeedbackContext::TReviewInfo& b) {
                 return a.GetTime() < b.GetTime();
             });

        for (auto iter = reviews.rbegin(); iter != reviews.rend(); ++iter) {
            if (curTime) {
                if (curReview != iter->GetText()) {
                    // it is new review, let's return first time of last review
                    return TInstant::MilliSeconds(curTime);
                }
            }
            curTime = iter->GetTime();
            curReview = iter->GetText();
        }

        // it is first time of review (in case Map contains one Review duplicates)
        // or Zero (in case Map is empty)
        return TInstant::MilliSeconds(curTime);
    }

    const TFeedbackAggregator::TPhotosList& TFeedbackAggregator::GetObjectPhotos(TStringBuf objectName) const {
        const auto& objects = Context_.GetPhotosData().GetObjects();
        auto iter = objects.find(TString(objectName));

        if (iter != objects.end()) {
            return iter->second.GetPhotos();
        }

        static ::google::protobuf::RepeatedPtrField<TProtoStringType> empty;
        return empty;
    }

    void TFeedbackAggregator::Normalize() {
        for (auto& table : *Context_.MutableData()->MutableTables()) {
            if (table.second.GetRows().empty()) {
                continue;
            }

            auto& objRows = *table.second.MutableRows();

            for (auto it = objRows.begin(); it != objRows.end();) {
                ui64 maxTime = 0;

                auto& objProps = *it->second.MutableProps();

                for (auto propsIt = objProps.begin(); propsIt != objProps.end();) {
                    if (propsIt->second.GetValue().empty()) {
                        objProps.erase(propsIt++);
                    } else {
                        maxTime = Max(maxTime, propsIt->second.GetTime());
                        ++propsIt;
                    }
                }

                if (it->second.GetProps().size() <= 1) {
                    objRows.erase(it++);
                } else {
                    ++it;
                }
            }
        }
    }

    bool TFeedbackAggregator::CheckUserId(const NJson::TJsonValue& feedback) {
        // XXX tmp
        if (!Context_.GetAppId()) {
            Context_.SetAppId(feedback["app"].GetString());
        }

        if (!Context_.GetUserId()) {
            Context_.SetUserId(feedback["userId"].GetString());
            return true;
        }

        return Context_.GetUserId() == feedback["userId"].GetString();
    }

    bool TFeedbackAggregator::FeedbackConstraints(const NJson::TJsonValue& feedback, TErrorDescr& error) {
        if (feedback.IsNull()) {
            error.AppendId("null") << "the given feedback is null";
            return false;
        }

        TStringBuf id = feedback["updateId"].GetString();

        if (!CheckUserId(feedback)) {
            error.AppendId(id) << "a new feedback's user '"
                               << feedback["userId"].GetString()
                               << "' doesn't match a prev one '"
                               << Context_.GetUserId();
            return false;
        }

        // common checks
        if ("ugcupdate" != feedback["type"].GetString()) {
            error.AppendId(id) << "type is not an 'ugcupdate' ("
                               << feedback["type"].GetString() << ')';
            return false;
        }

        if (NFeedbackReaderUtils::PROTOCOL_VERSION != feedback["version"].GetString()) {
            error.AppendId(id) << "version mismatch: our is '" << NFeedbackReaderUtils::PROTOCOL_VERSION
                               << "' != feedback's one '"
                               << feedback["version"].GetString() << '\'';
            return false;
        }

        return true;
    }

    bool TFeedbackAggregator::AppendFeedback(const NJson::TJsonValue& feedback, TErrorDescr& error,
                                             const TAppendOptions& options) {
        if (!FeedbackConstraints(feedback, error)) {
            return false;
        }

        const ui64 updateTime = feedback["time"].GetUInteger();
        if (!Context_.GetStartAt() || Context_.GetStartAt() > updateTime) {
            Context_.SetStartAt(updateTime);
        }
        if (!Context_.GetFinishAt() || updateTime > Context_.GetFinishAt()) {
            Context_.SetFinishAt(updateTime);
        }

        for (const auto& tableKV : feedback.GetMap()) {
            if (!tableKV.second.IsArray()) {
                continue;
            }

            auto& table = (*Context_.MutableData()->MutableTables())[tableKV.first];
            for (const auto& row : tableKV.second.GetArray()) {
                auto rowKey = row[Context_.GetTableKeyName()].GetString(); // XXX check for key?!
                auto& object = (*table.MutableRows())[rowKey];
                bool reviewOverallChanged = false;
                bool reviewFeedbackIdChanged = false;
                bool ratingOverallChanged = false;

                for (const auto& colVal : row.GetMap()) {
                    if (colVal.second.IsMap() || colVal.second.IsArray()) {
                        // field value is not simple type -> invalid update, need to skip it
                        continue;
                    }

                    TString value = colVal.second.GetStringRobust();
                    if (options.FilterSkippedReviews_ && colVal.first == "reviewOverall" &&
                        value == NFeedbackReaderUtils::SKIPPED_REVIEW_TEXT) {
                        continue;
                    }

                    if (options.FixInvalidRatings_ && colVal.first == "ratingOverall") {
                        auto fixedRating = NFeedbackReaderUtils::FixRating(value);
                        if (fixedRating.Empty()) {
                            continue;
                        }
                        value = *fixedRating;
                    }

                    if (options.AddReviewsTime_ && colVal.first == "reviewOverall") {
                        auto& reviews = (*Context_.MutableReviewsData()->MutableObjects())[rowKey];
                        auto* reviewInfo = reviews.MutableReviews()->Add();
                        reviewInfo->SetTime(updateTime);
                        reviewInfo->SetText(value);
                    }

                    if (options.AddPhotos_ && colVal.first == "photoId") {
                        (*Context_.MutablePhotosData()->MutableObjects())[rowKey].AddPhotos(value);
                    }

                    auto& record = (*object.MutableProps())[colVal.first];
                    if (record.GetTime() < updateTime) {
                        record.SetTime(updateTime);
                        record.SetValue(value);

                        if (colVal.first == "reviewOverall") {
                            reviewOverallChanged = true;
                        } else if (colVal.first == "ratingOverall") {
                            ratingOverallChanged = true;
                        } else if (colVal.first == "feedbackId") {
                            reviewFeedbackIdChanged = true;
                        }
                    }
                }

                // we need fix empty feedbackId for each update with reviewOverall or ratingOverall
                if (reviewOverallChanged && !reviewFeedbackIdChanged) {
                    PatchFeedbackId(rowKey, updateTime, feedback, object);
                } else if (ratingOverallChanged && !reviewFeedbackIdChanged) {
                    auto feedbackId = object.MutableProps()->find("feedbackId");
                    if (feedbackId == object.MutableProps()->end() || feedbackId->second.GetValue().empty()) {
                        PatchFeedbackId(rowKey, updateTime, feedback, object);
                    }
                }

            }
        }

        return true;
    }

    size_t TFeedbackAggregator::LoadFromUserData(const TString& userData, TFeedbackAggregator::TErrorDescr& error,
                                                 const TAppendOptions& options) {

        NJson::TJsonValue userDataJson;
        NJson::ReadJsonFastTree(userData, &userDataJson);

        return LoadFromUserData(userDataJson, error, options);
    }

    size_t TFeedbackAggregator::LoadFromUserData(const NJson::TJsonValue& userData, TFeedbackAggregator::TErrorDescr& error,
                                                 const TAppendOptions& options) {
        if (userData["type"].GetString() != "user-data") {
            error << "type is not a 'user-data'";
            return 0;
        }

        size_t feedbackCounter = 0;
        for (const NJson::TJsonValue& update : userData["updates"].GetArray()) {
            NJson::TJsonValue value;
            NJson::ReadJsonFastTree(update["value"].GetString(), &value);

            TFeedbackAggregator::TErrorDescr feedbackError;
            if (!AppendFeedback(value, feedbackError, options)) {
                error.PushUpdate(std::move(feedbackError));
                // just write the error and continue working!!!
            } else {
                ++feedbackCounter;
            }
        }

        Normalize();

        return feedbackCounter;
    }

} // namespace NUgc
