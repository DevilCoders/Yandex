#pragma once

#include "match_normalizers.h"
#include "feature_description.h"
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NStringMatchTracker {
    namespace NPrivate {
        class TCalcerState {
        public:
            static const size_t MaxLen = 1 << 31;
            size_t QueryLen = 0;
            size_t BestDocLen = MaxLen;
            size_t BestMatch = 0;

            void Y_FORCE_INLINE NewQuery(size_t queryLen) {
                QueryLen = queryLen;
                BestDocLen = MaxLen;
                BestMatch = 0;
            }

            void Y_FORCE_INLINE ResetMatch() {
                BestDocLen = MaxLen;
                BestMatch = 0;
            }
        };
    }

    class ICalcer {
    public:
        virtual ECalcer GetCalcerType() const = 0;
        virtual void NewQuery(const TString& query) = 0;
        virtual void ProcessDoc(const char* docBeg, const char* docEnd) = 0;
        virtual void Reset() = 0;
        virtual void CalcAllFeatures(TVector<TFeatureDescription>& features) const = 0;
        virtual size_t GetRawMatch() const = 0;
        virtual bool IsInited() const = 0;
        virtual ~ICalcer() {}
    };

    class TCalcer : public ICalcer {
    protected:
        NPrivate::TCalcerState State;
        bool Inited = false;

    public:
    #define CALC_FEATURE_METHOD(FNAME)\
        TFeatureDescription NormalizeAs##FNAME() const {\
            TFeatureDescription f;\
            f.FeatureId.Calcer = GetCalcerType();\
            Y_ASSERT(f.FeatureId.Calcer != ECalcer::None);\
            f.FeatureId.Normalizer = TNormalizer##FNAME::Name();\
            Y_ASSERT(f.FeatureId.Normalizer != ENormalizer::None);\
            f.Value = TNormalizer##FNAME::Calc(State.BestMatch, State.QueryLen, State.BestDocLen);\
            return f;\
        }

        CALC_FEATURE_METHOD(QueryLen)
        CALC_FEATURE_METHOD(DocLen)
        CALC_FEATURE_METHOD(MaxLen)
        CALC_FEATURE_METHOD(SumLen)
        CALC_FEATURE_METHOD(SimilarityFixed)

    #undef CALC_FEATURE_METHOD

        size_t GetRawMatch() const override {
            return State.BestMatch;
        }

        void NewQuery(const TString&) override {
            Inited = true;
        }

        bool IsInited() const override {
            return Inited;
        }

    public:
        void CalcAllFeatures(TVector<TFeatureDescription>& features) const override {
            features.push_back(NormalizeAsQueryLen());
            features.push_back(NormalizeAsDocLen());
            features.push_back(NormalizeAsMaxLen());
            features.push_back(NormalizeAsSumLen());
            features.push_back(NormalizeAsSimilarityFixed());
        }

        ~TCalcer() override {}
    };
}
