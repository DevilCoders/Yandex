#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/list.h>
#include <util/string/builder.h>

namespace NUgc {
    namespace NFeedbackReaderUtils {
        static const TStringBuf PROTOCOL_VERSION("1.0");
        static const TStringBuf SKIPPED_REVIEW_TEXT("skip");

        TMaybe<TString> FixRating(const TString& rating);

        TString BuildUserReviewFeedbackId(const TString& userId, const TString& parentId, const TString& contextId);

        class TErrorDescr {
        public:
            TErrorDescr() = default;

            TErrorDescr(TErrorDescr&& err);

            TErrorDescr(TStringBuf id);

            TErrorDescr& AppendId(TStringBuf id);

            template <typename T>
            TErrorDescr& operator<<(const T& obj) {
                Msg_ << obj;
                return *this;
            }

            operator bool() const;

            void PushUpdate(TErrorDescr&& update);

            NSc::TValue ToJson() const;

        private:
            TStringBuilder Msg_;
            TList<TErrorDescr> Updates_;
        };


        struct TAppendOptions {
            TAppendOptions() {}

#define DECLARE_FIELD_DEFAULT(Type, Name, Default)                \
            Type Name##_ {Default};                               \
            inline TAppendOptions& Name(const Type& v) {          \
                Name##_ = v;                                      \
                return *this;                                     \
            }

            DECLARE_FIELD_DEFAULT(bool, FilterSkippedReviews, false)

            DECLARE_FIELD_DEFAULT(bool, FixInvalidRatings, true)

            DECLARE_FIELD_DEFAULT(bool, AddFeedbackIds, false)

            DECLARE_FIELD_DEFAULT(bool, AddReviewsTime, false)

            DECLARE_FIELD_DEFAULT(bool, AddPhotos, false)

            DECLARE_FIELD_DEFAULT(bool, AddMeta, false)

            DECLARE_FIELD_DEFAULT(bool, UseKinopoiskRatingAsOurs, false)

#undef DECLARE_FIELD_DEFAULT
        };

    }
}
