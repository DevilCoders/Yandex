#pragma once

#include "detail.h"

#include <library/cpp/scheme/scheme.h>

#include <util/ysaveload.h>
#include <utility>
#include <utility>
#include <util/generic/strbuf.h>
#include <util/generic/utility.h>
#include <functional>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

#include <iterator>

namespace NScores {
    template <typename T = ui32>
    struct TTrueFalseScores {
        T TruePositive = 0;
        T FalsePositive = 0;
        T FalseNegative = 0;
        T TrueNegative = 0;

        NSc::TValue ToJson() const {
            NSc::TValue value;
#define LIBRARY_SCORES_ADD_JSON_TRUE_FALSE_SCORES_FIELD(field) \
    value[TStringBuf(#field)] = field;
            LIBRARY_SCORES_ADD_JSON_TRUE_FALSE_SCORES_FIELD(TruePositive)
            LIBRARY_SCORES_ADD_JSON_TRUE_FALSE_SCORES_FIELD(FalsePositive)
            LIBRARY_SCORES_ADD_JSON_TRUE_FALSE_SCORES_FIELD(FalseNegative)
            LIBRARY_SCORES_ADD_JSON_TRUE_FALSE_SCORES_FIELD(TrueNegative)
#undef LIBRARY_SCORES_ADD_JSON_TRUE_FALSE_SCORES_FIELD
            return value;
        }
    };

    template <typename T, typename U>
    inline TTrueFalseScores<T>& operator+=(
        TTrueFalseScores<T>& lhs, const TTrueFalseScores<U>& rhs) {
        lhs.TruePositive += rhs.TruePositive;
        lhs.FalsePositive += rhs.FalsePositive;
        lhs.FalseNegative += rhs.FalseNegative;
        lhs.TrueNegative += rhs.TrueNegative;

        return lhs;
    }

    template <typename T, typename U>
    inline TTrueFalseScores<T>& operator-=(
        TTrueFalseScores<T>& lhs, const TTrueFalseScores<U>& rhs) {
        lhs.TruePositive -= rhs.TruePositive;
        lhs.FalsePositive -= rhs.FalsePositive;
        lhs.FalseNegative -= rhs.FalseNegative;
        lhs.TrueNegative -= rhs.TrueNegative;

        return lhs;
    }

    template <typename T, typename U>
    inline TTrueFalseScores<T> operator+(
        TTrueFalseScores<T> lhs, const TTrueFalseScores<U>& rhs) {
        return lhs += rhs;
    }

    template <typename T, typename U>
    inline TTrueFalseScores<T> operator-(
        TTrueFalseScores<T> lhs, const TTrueFalseScores<U>& rhs) {
        return lhs -= rhs;
    }

    template <typename T, typename C = ui32>
    struct TBinaryClassificationScores {
        TTrueFalseScores<C> TrueFalse;
        T Prevalence = 0;
        T Precision = 0;               //! Also known as PPV (positive predictive value)
        T FalseDiscoveryRate = 0;      //! Also known as FDR
        T FalseOmissionRate = 0;       //! Also known as FOR
        T NegativePredictiveValue = 0; //! Also known as NPV
        T PositiveLikelihoodRatio = 0; //! Also known as LR+
        T NegativeLikelihoodRatio = 0; //! Also known as LR-
        T DiagnosticOddsRatio = 0;     //! Also known as DOR
        T Recall = 0;                  //! Also known as TPR (true positive rate) and Sensitivity
        T FalseNegativeRate = 0;       //! Also known as FNR
        T FalsePositiveRate = 0;       //! Also known as FPR and Fall-out
        T TrueNegativeRate = 0;        //! Also known as TNR, Specificity and SPC
        T Accuracy = 0;                //! Also known as ACC
        T FOne = 0;

        NSc::TValue ToJson() const {
            NSc::TValue value;
            value[TStringBuf("TrueFalse")] = TrueFalse.ToJson();
#define LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(field) \
    value[TStringBuf(#field)] = field;
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(Prevalence)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(Precision)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(FalseDiscoveryRate)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(FalseOmissionRate)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(NegativePredictiveValue)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(PositiveLikelihoodRatio)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(NegativeLikelihoodRatio)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(DiagnosticOddsRatio)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(Recall)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(FalseNegativeRate)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(FalsePositiveRate)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(TrueNegativeRate)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(Accuracy)
            LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD(FOne)
#undef LIBRARY_SCORES_ADD_JSON_BINARY_CLASSIFICATION_SCORES_FIELD
            return value;
        }
    };

    template <typename T>
    class TDefaultIsPositiveSample {
        static const T NEGATIVE_SAMPLE;

    public:
        //! @sample != T()
        inline bool operator()(const T& sample) const {
            return NEGATIVE_SAMPLE != sample;
        }
    };

    template <typename T>
    const T TDefaultIsPositiveSample<T>::NEGATIVE_SAMPLE = T();

    template <typename T, typename U = T, typename C = ui32>
    class TBinaryClassificationScorer {
    public:
        using TPredicted = T;
        using TExpected = U;
        using TCounter = C;
        using TScores = TTrueFalseScores<TCounter>;
        using TIsPositiveSampleChecker = std::function<bool(const TPredicted&)>;

        TBinaryClassificationScorer()
            : Scores_()
            , Total_()
            , IsPositiveSample_(TDefaultIsPositiveSample<T>())
        {
        }

        template <typename F>
        explicit TBinaryClassificationScorer(F&& isPositiveSampleChecker)
            : Scores_()
            , Total_()
            , IsPositiveSample_(std::forward<F>(isPositiveSampleChecker))
        {
        }

        template <typename F>
        inline TBinaryClassificationScorer& SetChecker(F&& checker) {
            IsPositiveSample_ = std::forward<F>(checker);
            return *this;
        }

        inline TIsPositiveSampleChecker GetChecker() const {
            return IsPositiveSample_;
        }

        TBinaryClassificationScorer& Push(const TPredicted& predicted, const TExpected& expected) {
            ++Total_;
            if (IsPositiveSample_(expected)) {
                if (IsPositiveSample_(predicted))
                    ++Scores_.TruePositive;
                else
                    ++Scores_.FalseNegative;
            } else {
                if (IsPositiveSample_(predicted))
                    ++Scores_.FalsePositive;
                else
                    ++Scores_.TrueNegative;
            }

            return *this;
        }

        TBinaryClassificationScorer& Pop(const TPredicted& predicted, const TExpected& expected) {
            Y_ASSERT(Total_ > 0);

            --Total_;
            if (IsPositiveSample_(expected)) {
                if (IsPositiveSample_(predicted))
                    --Scores_.TruePositive;
                else
                    --Scores_.FalseNegative;
            } else {
                if (IsPositiveSample_(predicted))
                    --Scores_.FalsePositive;
                else
                    --Scores_.TrueNegative;
            }

            return *this;
        }

        TBinaryClassificationScorer Invert() const {
            TBinaryClassificationScorer inverted = *this;

            auto checker = inverted.GetChecker();
            inverted.SetChecker([checker](const TPredicted& value) { return !checker(value); });
            DoSwap(inverted.Scores_.TruePositive, inverted.Scores_.TrueNegative);
            DoSwap(inverted.Scores_.FalseNegative, inverted.Scores_.FalsePositive);
            return inverted;
        }

        template <typename TIt, typename TItOther>
        inline TBinaryClassificationScorer& PushMany(
            TIt predictedBegin, TIt predictedEnd, TItOther expectedBegin) {
            for (; predictedBegin != predictedEnd; ++predictedBegin, ++expectedBegin)
                Push(*predictedBegin, *expectedBegin);

            return *this;
        }

        template <typename TIt, typename TItOther>
        inline TBinaryClassificationScorer& PopMany(
            TIt predictedBegin, TIt predictedEnd, TItOther expectedBegin) {
            for (; predictedBegin != predictedEnd; ++predictedBegin, ++expectedBegin)
                Pop(*predictedBegin, *expectedBegin);

            return *this;
        }

        template <typename TOther, typename UOther, typename COther>
        inline TBinaryClassificationScorer& operator+=(
            const TBinaryClassificationScorer<TOther, UOther, COther>& other) {
            Scores_ += other.Scores_;
            Total_ += other.Total_;

            return *this;
        }

        template <typename TOther, typename UOther, typename COther>
        inline TBinaryClassificationScorer& operator-=(
            const TBinaryClassificationScorer<TOther, UOther, COther>& other) {
            Y_ASSERT(Total_ >= other.Total_);

            Scores_ -= other.Scores_;
            Total_ -= other.Total_;

            return *this;
        }

        inline const TScores& TrueFalseScores() const {
            return Scores_;
        }

        inline TCounter Count() const {
            return Total_;
        }

        template <typename R>
        inline R Prevalence() const {
            return NDetail::DivideOrZero<R>(
                Scores_.TruePositive + Scores_.FalseNegative,
                Total_);
        }

        //! Also known as PPV (positive predictive value)
        template <typename R>
        inline R Precision() const {
            return NDetail::DivideOrZero<R>(
                Scores_.TruePositive,
                Scores_.TruePositive + Scores_.FalsePositive);
        }

        //! Also known as Precision
        template <typename R>
        inline R PositivePredictiveValue() const {
            return Precision<R>();
        }

        template <typename R>
        inline R FalseDiscoveryRate() const {
            return NDetail::DivideOrZero<R>(
                Scores_.FalsePositive,
                Scores_.TruePositive + Scores_.FalsePositive);
        }

        template <typename R>
        inline R FalseOmissionRate() const {
            return NDetail::DivideOrZero<R>(
                Scores_.FalseNegative,
                Scores_.FalseNegative + Scores_.TrueNegative);
        }

        template <typename R>
        inline R NegativePredictiveValue() const {
            return NDetail::DivideOrZero<R>(
                Scores_.TrueNegative,
                Scores_.FalseNegative + Scores_.TrueNegative);
        }

        template <typename R>
        inline R PositiveLikelihoodRatio() const {
            return NDetail::DivideOrZero<R>(
                Scores_.TruePositive,
                Scores_.FalsePositive);
        }

        template <typename R>
        inline R NegativeLikelihoodRatio() const {
            return NDetail::DivideOrZero<R>(
                Scores_.FalseNegative,
                Scores_.TrueNegative);
        }

        template <typename R>
        inline R DiagnosticOddsRatio() const {
            return NDetail::DivideOrZero<R>(
                PositiveLikelihoodRatio<R>(),
                NegativeLikelihoodRatio<R>());
        }

        //! Also known as TPR (true positive rate) and Sensitivity
        template <typename R>
        inline R Recall() const {
            return NDetail::DivideOrZero<R>(
                Scores_.TruePositive,
                Scores_.TruePositive + Scores_.FalseNegative);
        }

        //! Also known as Recall and Sensitivity
        template <typename R>
        inline R TruePositiveRate() const {
            return Recall<R>();
        }

        //! Also known as Recall and TPR (true positive rate)
        template <typename R>
        inline R Sensitivity() const {
            return Sensitivity<R>();
        }

        template <typename R>
        inline R FalseNegativeRate() const {
            return NDetail::DivideOrZero<R>(
                Scores_.FalseNegative,
                Scores_.TruePositive + Scores_.FalseNegative);
        }

        //! Also known as FPR and Fall-out
        template <typename R>
        inline R FalsePositiveRate() const {
            return NDetail::DivideOrZero<R>(
                Scores_.FalsePositive,
                Scores_.FalsePositive + Scores_.TrueNegative);
        }

        //! Also known as FPR (false positive rate)
        template <typename R>
        inline R FallOut() const {
            return FalsePositiveRate<R>();
        }

        //! Also known as TNR, Specificity and SPC
        template <typename R>
        inline R TrueNegativeRate() const {
            return NDetail::DivideOrZero<R>(
                Scores_.TrueNegative,
                Scores_.FalsePositive + Scores_.TrueNegative);
        }

        //! Also known as TPR (true positive rate)
        template <typename R>
        inline R Specificity() const {
            return TrueNegativeRate<R>();
        }

        template <typename R>
        inline R Accuracy() const {
            return NDetail::DivideOrZero<R>(
                Scores_.TruePositive + Scores_.TrueNegative,
                Total_);
        }

        template <typename W, typename R = W>
        inline R F(const W beta) const {
            const R precision = Precision<R>();
            const R recall = Recall<R>();
            return NDetail::DivideOrZero<R>(
                (beta * beta + 1) * precision * recall,
                beta * beta * precision + recall);
        }

        template <typename R>
        inline R FOne() const {
            return F<R>(R(1));
        }

        template <typename R>
        TBinaryClassificationScores<R, TCounter> Get() const {
            TBinaryClassificationScores<R, TCounter> scores;

            scores.TrueFalse = Scores_;
            scores.Prevalence = Prevalence<R>();
            scores.Precision = Precision<R>();
            scores.FalseDiscoveryRate = FalseDiscoveryRate<R>();
            scores.FalseOmissionRate = FalseOmissionRate<R>();
            scores.NegativePredictiveValue = NegativePredictiveValue<R>();
            scores.PositiveLikelihoodRatio = PositiveLikelihoodRatio<R>();
            scores.NegativeLikelihoodRatio = NegativeLikelihoodRatio<R>();
            scores.DiagnosticOddsRatio = DiagnosticOddsRatio<R>();
            scores.Recall = Recall<R>();
            scores.FalseNegativeRate = FalseNegativeRate<R>();
            scores.FalsePositiveRate = FalsePositiveRate<R>();
            scores.TrueNegativeRate = TrueNegativeRate<R>();
            scores.Accuracy = Accuracy<R>();
            scores.FOne = FOne<R>();

            return scores;
        }

        Y_SAVELOAD_DEFINE(Scores_, Total_);

    private:
        TScores Scores_;
        TCounter Total_;
        TIsPositiveSampleChecker IsPositiveSample_;
    };

    template <typename T, typename U, typename C,
              typename TOther, typename UOther, typename COther>
    inline TBinaryClassificationScorer<T, U, C> operator+(
        TBinaryClassificationScorer<T, U, C> lhs,
        const TBinaryClassificationScorer<TOther, UOther, COther>& rhs) {
        return lhs += rhs;
    }

    template <typename T, typename U, typename C,
              typename TOther, typename UOther, typename COther>
    inline TBinaryClassificationScorer<T, U, C> operator-(
        TBinaryClassificationScorer<T, U, C> lhs,
        const TBinaryClassificationScorer<TOther, UOther, COther>& rhs) {
        return lhs -= rhs;
    }

    template <typename Result, typename TIt, typename TItOther,
              typename F = TDefaultIsPositiveSample<typename std::iterator_traits<TIt>::value_type>>
    TBinaryClassificationScores<Result, size_t> CalculateBinaryClassificationScoresForSequence(
        TIt predictedBegin, TIt predictedEnd, TItOther expectedBegin,
        F&& isPositiveSampleChecker = F()) {
        using TPredicted = typename std::iterator_traits<TIt>::value_type;
        using TExpected = typename std::iterator_traits<TItOther>::value_type;

        return TBinaryClassificationScorer<TPredicted, TExpected, size_t>(
                   std::forward<F>(isPositiveSampleChecker))
            .PushMany(
                predictedBegin, predictedEnd,
                expectedBegin)
            .template Get<Result>();
    }

    template <typename Result, typename TIt, typename TItOther, typename W,
              typename F = TDefaultIsPositiveSample<typename std::iterator_traits<TIt>::value_type>>
    TBinaryClassificationScores<Result, size_t> CalculateFForSequence(
        TIt predictedBegin, TIt predictedEnd, TItOther expectedBegin, const W beta,
        F&& isPositiveSampleChecker = F()) {
        using TPredicted = typename std::iterator_traits<TIt>::value_type;
        using TExpected = typename std::iterator_traits<TItOther>::value_type;

        return TBinaryClassificationScorer<TPredicted, TExpected, size_t>(
                   std::forward<F>(isPositiveSampleChecker))
            .PushMany(
                predictedBegin, predictedEnd,
                expectedBegin)
            .template F<Result>(beta);
    }

#define LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(score)                                    \
    template <typename Result, typename TIt, typename TItOther,                                      \
              typename F = TDefaultIsPositiveSample<typename std::iterator_traits<TIt>::value_type>> \
    inline Result Calculate##score##ForSequence(                                                     \
        TIt predictedBegin, TIt predictedEnd, TItOther expectedBegin,                                \
        F&& isPositiveSampleChecker = F()) {                                                         \
        using TPredicted = typename std::iterator_traits<TIt>::value_type;                           \
        using TExpected = typename std::iterator_traits<TItOther>::value_type;                       \
                                                                                                     \
        return TBinaryClassificationScorer<TPredicted, TExpected, size_t>(                           \
                   std::forward<F>(isPositiveSampleChecker))                                         \
            .PushMany(                                                                               \
                predictedBegin, predictedEnd,                                                        \
                expectedBegin)                                                                       \
            .template score<Result>();                                                               \
    }

    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(Prevalence)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(Precision)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(PositivePredictiveValue)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(FalseDiscoveryRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(FalseOmissionRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(NegativePredictiveValue)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(PositiveLikelihoodRatio)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(NegativeLikelihoodRatio)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(DiagnosticOddsRatio)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(Recall)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(TruePositiveRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(Sensitivity)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(FalseNegativeRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(FalsePositiveRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(FallOut)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(TrueNegativeRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(Specificity)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(Accuracy)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE(FOne)

#undef LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_SEQUENCE

    template <typename Result, typename T, typename U,
              typename F = TDefaultIsPositiveSample<typename T::value_type>>
    TBinaryClassificationScores<Result, size_t> CalculateBinaryClassificationScores(
        const T& predicted, const U& expected,
        F&& isPositiveSampleChecker = F()) {
        Y_ASSERT(predicted.size() == expected.size());

        return CalculateBinaryClassificationScoresForSequence<Result>(
            predicted.begin(), predicted.end(),
            expected.begin(),
            std::forward<F>(isPositiveSampleChecker));
    }

    template <typename Result, typename T, typename U, typename W,
              typename F = TDefaultIsPositiveSample<typename T::value_type>>
    inline Result CalculateF(
        const T& predicted, const U& expected, const W beta,
        F&& isPositiveSampleChecker = F()) {
        Y_ASSERT(predicted.size() == expected.size());

        return CalculateFForSequence<Result>(
            predicted.begin(), predicted.end(),
            expected.begin(),
            beta,
            std::forward<F>(isPositiveSampleChecker));
    }

#define LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(score)           \
    template <typename Result, typename T, typename U,                       \
              typename F = TDefaultIsPositiveSample<typename T::value_type>> \
    inline Result Calculate##score(                                          \
        const T& predicted, const U& expected,                               \
        F&& isPositiveSampleChecker = F()) {                                 \
        Y_ASSERT(predicted.size() == expected.size());                       \
                                                                             \
        return Calculate##score##ForSequence<Result>(                        \
            predicted.begin(), predicted.end(),                              \
            expected.begin(),                                                \
            std::forward<F>(isPositiveSampleChecker));                       \
    }

    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(Prevalence)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(Precision)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(PositivePredictiveValue)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(FalseDiscoveryRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(FalseOmissionRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(NegativePredictiveValue)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(PositiveLikelihoodRatio)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(NegativeLikelihoodRatio)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(DiagnosticOddsRatio)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(Recall)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(TruePositiveRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(Sensitivity)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(FalseNegativeRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(FalsePositiveRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(FallOut)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(TrueNegativeRate)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(Specificity)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(Accuracy)
    LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER(FOne)

#undef LIBRARY_SCORES_DEFINE_CALCULATE_SCORE_FOR_CONTAINER

}
