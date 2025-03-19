#include "utils.h"

#include <kernel/ugc/security/lib/record_identifier.h>

#include <util/generic/hash.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NUgc {
    namespace NFeedbackReaderUtils {

        namespace {
            // XXX: some ratings in ugcdb are not numbers by historical reasons but represent valid values.
            // Here is code which converts them to numbers. Also there are ratings with 0 value which came from
            // maps and represent absent rating, convert them to delete-thumbstones
            const THashMap<TStringBuf, TString> STR_RATING_MAPPING = {
                    {"bad",  "1"},
                    {"good", "5"},
            };
        }

        TString BuildUserReviewFeedbackId(const TString& userId, const TString& parentId, const TString& contextId) {
            NSecurity::TRecordIdentifierBundle bundle;
            bundle.SetNamespace("UserFeedback");
            bundle.SetUserId(userId);
            bundle.SetParentId(parentId);
            bundle.SetContextId(contextId);
            bundle.SetClientEntropy("serp");
            return NUgc::NSecurity::GenerateRecordIdentifierByHash(bundle);
        }

        TMaybe<TString> FixRating(const TString& rating) {

            // empty rating means removed
            if (rating.empty()) {
                return rating;
            }

            // try to fix string ratings
            auto it = STR_RATING_MAPPING.find(rating);
            if (it != STR_RATING_MAPPING.end()) {
                return it->second;
            }

            // try to fix zero ratings
            float ratingValue = 0;
            if (TryFromString(rating, ratingValue)) {
                return ratingValue > 0.01 ? rating : "";
            }

            // rating is fatally invalid
            return Nothing();
        }

        TErrorDescr::TErrorDescr(TErrorDescr&& error) {
            Msg_.swap(error.Msg_);
            Updates_.swap(error.Updates_);
        }

        TErrorDescr::TErrorDescr(TStringBuf id) {
            AppendId(id);
        }

        TErrorDescr& TErrorDescr::AppendId(TStringBuf id) {
            Msg_ << '[' << id << "] ";
            return *this;
        }

        void TErrorDescr::PushUpdate(TErrorDescr&& error) {
            Updates_.push_back(std::move(error));
        }

        TErrorDescr::operator bool() const {
            return Msg_ || Updates_;
        }

        NSc::TValue TErrorDescr::ToJson() const {
            NSc::TValue error;
            if (Msg_) {
                error[TStringBuf("msg")].SetString(Msg_);
            }

            for (const TErrorDescr& update : Updates_) {
                NSc::TValue theUpdateJson = update.ToJson();
                if (!theUpdateJson.IsNull()) {
                    error[TStringBuf("updates")].Push(theUpdateJson);
                }
            }

            return error;
        }
    }
}
