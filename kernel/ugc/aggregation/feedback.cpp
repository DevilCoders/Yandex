#include "feedback.h"

#include <util/datetime/base.h>
#include <util/string/builder.h>
#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>

namespace NUgc {
    namespace {
        void PatchFeedbackId(const TString& key, const TInstant& updateTime, const NSc::TValue& feedback,
                             TFeedbackReader::TObjectProps& object, bool setGeneratedByRating) {
            // find old or current review text
            auto iter = object.find("reviewOverall");
            TString review = iter == object.end() ? TString() : iter->second.Value;

            // build feedbackId on (userId, orgId, reviewText)
            auto& reviewFeedbackId = object["feedbackId"];
            reviewFeedbackId.Time = updateTime;
            reviewFeedbackId.Value = BuildUserReviewFeedbackId(
                    TString(feedback["userId"].GetString()),
                    key,
                    review
            );
            reviewFeedbackId.IsGeneratedByRating = setGeneratedByRating;
        }

        TMaybe<TString> LookupByObjectIdAndTimestamp(
            const THashMap<TString, TMap<TInstant, TString>>& data,
            const TStringBuf& objectName,
            const TInstant& t)
        {
            TMaybe<TString> res;
            const auto itObject = data.find(objectName);
            if (itObject != data.end()) {
                const auto& innerMap = (*itObject).second;
                const auto itInner = innerMap.find(t);
                if (itInner != innerMap.end()) {
                    res = (*itInner).second;
                }
            }
            return res;
        }
    }

    TString BuildUserReviewFeedbackId(const TString& userId, const TString& parentId, const TString& contextId) {
        return NFeedbackReaderUtils::BuildUserReviewFeedbackId(userId, parentId, contextId);
    }

    TFeedbackReader::TFeedbackReader()
            : TableKeyName_("key") {
    }

    TFeedbackReader::TFeedbackReader(TStringBuf tableKeyName)
            : TableKeyName_(tableKeyName) {
    }

    const TFeedbackReader::TObjectProps* TFeedbackReader::GetObjectProps(TStringBuf tableName, TStringBuf objectName) const {
        const auto table = Data_.find(tableName);
        if (table == Data_.end()) {
            return nullptr;
        }

        const auto object = table->second.find(objectName);
        return object != table->second.end() ? &object->second : nullptr;
    }

    TMaybe<TStringBuf> TFeedbackReader::Value(TStringBuf tableName, TStringBuf objectName, TStringBuf columnName) const {
        const TObjectProps* props = GetObjectProps(tableName, objectName);
        if (!props) {
            return Nothing();
        }

        const auto column = props->find(columnName);
        if (column == props->end()) {
            return Nothing();
        }

        return TStringBuf(column->second.Value);
    }

    TInstant TFeedbackReader::GetObjectReviewTime(TStringBuf objectName) const {
        const auto object = ReviewsData_.find(objectName);
        if (object == ReviewsData_.end()) {
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

        auto& reviews = object->second; // Map<reviewTime, reviewText>
        TInstant curTime = TInstant::Zero();
        TString curReview;

        for (auto iter = reviews.crbegin(); iter != reviews.crend(); ++iter) {
            if (curTime) {
                if (curReview != iter->second) {
                    // it is new review, let's return first time of last review
                    return curTime;
                }
            }
            curTime = iter->first;
            curReview = iter->second;
        }

        // it is first time of review (in case Map contains one Review duplicates)
        // or Zero (in case Map is empty)
        return curTime;
    }

    TInstant TFeedbackReader::GetObjectRatingTime(TStringBuf objectName) const {
        if (const auto* object = MapFindPtr(RatingsData_, objectName)) {
            return object->rbegin()->first;
        }
        return TInstant::Zero();
    }

    TMaybe<TString> TFeedbackReader::GetReviewByTimestamp(TStringBuf objectName, const TInstant& t) const {
        return LookupByObjectIdAndTimestamp(ReviewsData_, objectName, t);
    }

    TMaybe<TString> TFeedbackReader::GetFeedbackIdByTimestamp(TStringBuf objectName, const TInstant& t) const {
        return LookupByObjectIdAndTimestamp(FeedbackIdData_, objectName, t);
    }

    NUgc::TReviewMeta TFeedbackReader::GetObjectReviewMeta(TStringBuf objectName) const {
        const auto& object = ReviewsMeta_.find(objectName);
        if (object == ReviewsMeta_.end()) {
            return TReviewMeta();
        }
        auto ptr = object->second.find(GetObjectReviewTime(objectName));
        if (ptr == object->second.end()) {
            return TReviewMeta();
        }
        return ptr->second;
    }

    const TVector<TString>& TFeedbackReader::GetObjectPhotos(TStringBuf objectName) const {
        auto iter = PhotosData_.find(objectName);
        if (iter != PhotosData_.end()) {
            return iter->second;
        }
        static TVector<TString> empty;
        return empty;
    }


    NSc::TValue TFeedbackReader::Aggregate(bool keepEmptyTombstone, bool sortEveryTable) const {
        NSc::TValue result;

        if (!FinishAt_.Defined() || !StartAt_.Defined()) {
            return result;
        }

        result[TStringBuf("type")].SetString(TStringBuf("ugcdata"));
        result[TStringBuf("version")].SetString(NFeedbackReaderUtils::PROTOCOL_VERSION);
        result[TStringBuf("userId")].SetString(UserId_);
        result[TStringBuf("appId")].SetString(AppId_);
        result[TStringBuf("time")].SetIntNumber(FinishAt_->MilliSeconds());
        result[TStringBuf("timeFrom")].SetIntNumber(StartAt_->MilliSeconds());
        if (UpdateIdBuilder_) {
            result[TStringBuf("updateId")].SetString(UpdateIdBuilder_->Result());
        }

        for (const auto& table : Data_) {
            if (!table.second.size()) {
                continue;
            }

            NSc::TValue jsonTable;
            for (const auto& obj : table.second) {
                if (!obj.second.size()) {
                    continue;
                }

                NSc::TValue jsonObject;
                TInstant maxTime = TInstant::Zero();
                for (const auto& col : obj.second) {
                    if (keepEmptyTombstone || col.second.Value) { // empty value means removed
                        jsonObject[col.first].SetString(col.second.Value);
                        maxTime = Max(maxTime, col.second.Time);
                    }
                }
                // if table contains only one value, it has to be key so the table is empty,
                // therefore we do not insert this table into result
                // TODO add additional check for key
                if (jsonObject.DictSize() > 1) {
                    jsonObject["time"] = maxTime.MilliSeconds();
                    jsonTable.Push(jsonObject);
                }
            }

            if (jsonTable.ArraySize()) {
                if (sortEveryTable) {
                    NSc::TArray& objects = jsonTable.GetArrayMutable();
                    Sort(objects, [](const NSc::TValue& left, const NSc::TValue& right) {
                        return left["time"].GetIntNumber() < right["time"].GetIntNumber();
                    });
                }

                result[table.first] = jsonTable;
            }
        }

        return result;
    }

    bool TFeedbackReader::CheckUserId(const NSc::TValue& feedback) {
        // XXX tmp
        if (!AppId_) {
            AppId_ = feedback[TStringBuf("app")].GetString();
        }

        if (!UserId_) {
            UserId_ = feedback[TStringBuf("userId")].GetString();
            return true;
        }

        return UserId_ == feedback[TStringBuf("userId")].GetString();
    }

    bool TFeedbackReader::FeedbackConstraints(const NSc::TValue& feedback, TErrorDescr& error) {
        if (feedback.IsNull()) {
            error.AppendId(TStringBuf("null")) << TStringBuf("the given feedback is null");
            return false;
        }

        TStringBuf id = feedback[TStringBuf("updateId")].GetString();

        if (!CheckUserId(feedback)) {
            error.AppendId(id) << TStringBuf("a new feedback's user '")
                               << feedback[TStringBuf("userId")].GetString()
                               << TStringBuf("' doesn't match a prev one '")
                               << UserId_;
            return false;
        }

        // common checks
        if (TStringBuf("ugcupdate") != feedback[TStringBuf("type")].GetString()) {
            error.AppendId(id) << TStringBuf("type is not an 'ugcupdate' (")
                               << feedback[TStringBuf("type")].GetString() << ')';
            return false;
        }

        if (NFeedbackReaderUtils::PROTOCOL_VERSION != feedback[TStringBuf("version")].GetString()) {
            error.AppendId(id) << TStringBuf("version mismatch: our is '") << NFeedbackReaderUtils::PROTOCOL_VERSION
                               << TStringBuf("' != feedback's one '")
                               << feedback[TStringBuf("version")].GetString() << '\'';
            return false;
        }

        return true;
    }

    void TFeedbackReader::SetUpdateIdBuilder(THolder<IUpdateIdBuilder> builder) {
        UpdateIdBuilder_.Swap(builder);
    }

    bool TFeedbackReader::AppendFeedback(const NSc::TValue& feedback, TErrorDescr& error, const TAppendOptions& options) {
        if (!FeedbackConstraints(feedback, error)) {
            return false;
        }

        const TInstant updateTime = TInstant::MilliSeconds(feedback[TStringBuf("time")]);
        if (!StartAt_.Defined() || *StartAt_ > updateTime) {
            StartAt_ = updateTime;
        }
        if (!FinishAt_.Defined() || updateTime > *FinishAt_) {
            FinishAt_ = updateTime;
        }

        bool hasReviewOverallRecord = false;
        TString objectId;

        for (const auto& tableKV : feedback.GetDict()) {
            if (!tableKV.second.IsArray()) {
                continue;
            }

            auto& table = Data_[tableKV.first];
            for (const auto& row : tableKV.second.GetArray()) {
                auto rowKey = row[TableKeyName_].GetString(); // XXX check for key?!
                if (rowKey) {
                    objectId = rowKey;
                }
                auto& object = table[rowKey];
                bool reviewOverallChanged = false;
                bool reviewFeedbackIdChanged = false;
                bool ratingOverallChanged = false;

                for (const auto& colVal : row.GetDict()) {
                    if (colVal.second.IsDict() || colVal.second.IsArray()) {
                        // field value is not simple type -> invalid update, need to skip it
                        continue;
                    }

                    TString propKey = TString{colVal.first};
                    TString propValue = colVal.second.ForceString();

                    if (options.FilterSkippedReviews_ && propKey == "reviewOverall" &&
                        propValue == NFeedbackReaderUtils::SKIPPED_REVIEW_TEXT) {
                        continue;
                    }

                    // Patch rating overall with kinopoisk rating on demand
                    if (options.UseKinopoiskRatingAsOurs_ && propKey == "kinopoiskRating") {
                        propKey = "ratingOverall";
                        ui64 kpRating;
                        if (TryFromString<ui64>(propValue, kpRating)) {
                            propValue = ToString((kpRating + 1) / 2);
                        } else {
                            propValue = "";
                        }
                    }

                    if (options.FixInvalidRatings_ && propKey == "ratingOverall") {
                        auto fixedRating = NFeedbackReaderUtils::FixRating(propValue);
                        if (fixedRating.Empty()) {
                            continue;
                        }
                        propValue = *fixedRating;
                    }

                    if (propKey == TStringBuf("reviewOverall")) {
                        hasReviewOverallRecord = true;
                        if (options.AddReviewsTime_) {
                            ReviewsData_[rowKey][updateTime] = propValue;
                        }

                        if (options.AddFeedbackIds_) {
                            TString feedbackId = row.Has("feedbackId")
                                ? TString(row["feedbackId"].GetString())
                                : BuildUserReviewFeedbackId(TString(feedback["userId"].GetString()), TString(rowKey), propValue);
                            FeedbackIdData_[rowKey][updateTime] = feedbackId;
                        }
                    }

                    if (options.AddReviewsTime_ && propKey == TStringBuf("ratingOverall")) {
                        RatingsData_[rowKey][updateTime] = propValue;
                    };

                    if (options.AddPhotos_ && propKey == TStringBuf("photoId")) {
                        PhotosData_[rowKey].push_back(propValue);
                    }

                    TRecord& record = object[propKey];
                    if (record.Time < updateTime
                        || (propKey == TStringBuf("feedbackId") && record.IsGeneratedByRating)) {

                        record.Time = updateTime;
                        record.Value = propValue;
                        record.IsGeneratedByRating = false;

                        if (propKey == TStringBuf("reviewOverall")) {
                            reviewOverallChanged = true;
                        } else if (propKey == TStringBuf("ratingOverall")) {
                            ratingOverallChanged = true;
                        } else if (propKey == TStringBuf("feedbackId")) {
                            reviewFeedbackIdChanged = true;
                        }
                    }
                }


                // we need fix empty feedbackId for each update with reviewOverall or ratingOverall
                if (reviewOverallChanged && !reviewFeedbackIdChanged) {
                    const auto* feedbackId = MapFindPtr(object, TStringBuf("feedbackId"));
                    if (!feedbackId || feedbackId->Value.empty() || feedbackId->Time < updateTime) {
                        PatchFeedbackId(TString(rowKey), updateTime, feedback, object, false);
                    }
                } else if (ratingOverallChanged && !reviewFeedbackIdChanged) {
                    const auto* feedbackId = MapFindPtr(object, TStringBuf("feedbackId"));
                    if (!feedbackId || feedbackId->Value.empty()) {
                        PatchFeedbackId(TString(rowKey), updateTime, feedback, object, true);
                    }
                }
            }
        }

        if (options.AddMeta_ && hasReviewOverallRecord) {
            const auto& dict = feedback.GetDict();
            if (dict.find("yandexTld") != dict.end()) {
                ReviewsMeta_[objectId][updateTime].SetYandexTLD(dict.Get("yandexTld").ForceString());
            }
            if (dict.find("countryCode") != dict.end()) {
                ReviewsMeta_[objectId][updateTime].SetCountryId(dict.Get("countryCode").ForceString());
            }
            if (dict.find("updateId") != dict.end()) {
                ReviewsMeta_[objectId][updateTime].SetUpdateId(dict.Get("updateId").ForceString());
            }
        }

        if (UpdateIdBuilder_) {
            UpdateIdBuilder_->OnFeedback(feedback);
        }

        return true;
    }

    size_t TFeedbackReader::LoadFromUserData(const NSc::TValue& userData, TFeedbackReader::TErrorDescr& error, const TAppendOptions& options) {
        if (userData[TStringBuf("type")].GetString() != TStringBuf("user-data")) {
            error << TStringBuf("type is not a 'user-data'");
            return 0;
        }

        size_t feedbackCounter = 0;
        for (const NSc::TValue& update : userData[TStringBuf("updates")].GetArray()) {
            const NSc::TValue value = NSc::TValue::FromJson(update[TStringBuf("value")].GetString());
            TFeedbackReader::TErrorDescr feedbackError;
            if (!AppendFeedback(value, feedbackError, options)) {
                error.PushUpdate(std::move(feedbackError));
                // just write the error and continue working!!!
            } else {
                ++feedbackCounter;
            }
        }

        return feedbackCounter;
    }

} // namespace NUgc
