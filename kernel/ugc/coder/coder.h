#pragma once

#include <library/cpp/scheme/scheme.h>

#include <kernel/signurl/signurl.h>
#include <kernel/ugc/proto/onto/object.pb.h>

namespace NUgc {
    class TFeedbackReader;

    namespace NCoder {
        static const size_t MAXSTRLEN = 1024 * 32;

        struct TRatingInfoParams {
            const NSignUrl::TKey& Key;
            const NUgc::TFeedbackReader* Feedback;
            const THashMap<TString, NUgc::NSchema::NOnto::TOntoObject>* Objects;
            TString AppId;
            TString UserId;
            TString VisitorId;
            TString ContextId;
            TString ObjectKey;
            TString Otype;

            TRatingInfoParams(const NSignUrl::TKey& key,
                              const NUgc::TFeedbackReader& feedback)
                : Key(key)
                , Feedback(&feedback)
                , Objects(nullptr)
            {}

            TRatingInfoParams(const NSignUrl::TKey& key,
                              const THashMap<TString, NUgc::NSchema::NOnto::TOntoObject>& objects)
                : Key(key)
                , Feedback(nullptr)
                , Objects(&objects)
            {}

            bool IsValid() const {
                return !AppId.empty()
                    && !UserId.empty()
                    && !VisitorId.empty()
                    && !ContextId.empty()
                    && !ObjectKey.empty()
                    && !Otype.empty();
            }
        };

        void Encode(NSc::TValue& val, const NSignUrl::TKey& key);
        void Decode(NSc::TValue& val, const NSignUrl::TKey& key);

        NSc::TValue GetRatingInfo(const TRatingInfoParams& params);
    } // namespace NCoder
} // namespace NUgc
