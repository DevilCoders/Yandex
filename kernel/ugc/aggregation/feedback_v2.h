#pragma once

#include "utils.h"
#include <kernel/ugc/aggregation/proto/aggregated_feedback.pb.h>

#include <library/cpp/json/writer/json_value.h>
#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/list.h>
#include <util/string/builder.h>

namespace NUgc {
    class TFeedbackAggregator {
    public:
        using TErrorDescr = NFeedbackReaderUtils::TErrorDescr;
        using TAppendOptions = NFeedbackReaderUtils::TAppendOptions;
        using TObjectProps = ::google::protobuf::Map<TProtoStringType, TFeedbackContext::TRecord>;
        using TPhotosList = ::google::protobuf::RepeatedPtrField<TProtoStringType>;

    public:
        TFeedbackAggregator();

        explicit TFeedbackAggregator(TStringBuf tableKeyName);

        explicit TFeedbackAggregator(const TFeedbackContext& context);

        size_t LoadFromUserData(const TString& userData, TFeedbackAggregator::TErrorDescr& error,
                                const TAppendOptions& options);

        size_t LoadFromUserData(const NJson::TJsonValue& userData, TFeedbackAggregator::TErrorDescr& error,
                                const TAppendOptions& options = TAppendOptions());

        TInstant GetObjectReviewTime(TStringBuf objectName) const;

        const TPhotosList& GetObjectPhotos(TStringBuf objectName) const;

        TMaybe<TStringBuf> Value(TStringBuf table, TStringBuf object, TStringBuf column) const;

        const TObjectProps* GetObjectProps(TStringBuf table, TStringBuf object) const;

        const TFeedbackContext GetContext() const {
            return Context_;
        }

        template <typename Container>
        size_t AppendFeedbacks(const Container& container, TErrorDescr& error,
                               const TAppendOptions& options = TAppendOptions()) {
            size_t counter = 0;

            for (const NJson::TJsonValue& feedback : container) {
                TFeedbackAggregator::TErrorDescr feedbackError;
                if (!AppendFeedback(feedback, feedbackError, options)) {
                    error.PushUpdate(std::move(feedbackError));
                } else {
                    ++counter;
                }
            }

            Normalize();

            return counter;
        }


    protected:
        bool AppendFeedback(const NJson::TJsonValue& feedback, TErrorDescr& error,
                            const TAppendOptions& options = TAppendOptions());

    private:
        bool CheckUserId(const NJson::TJsonValue& feedback);

        bool FeedbackConstraints(const NJson::TJsonValue& feedback, TErrorDescr& error);

        void Normalize();

    private:
        TFeedbackContext Context_;
    };
}
