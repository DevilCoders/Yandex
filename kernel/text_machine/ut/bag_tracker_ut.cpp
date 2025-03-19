#include <kernel/text_machine/parts/accumulators/bow_accumulator_parts.h>
#include <kernel/text_machine/parts/common/types.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTextMachine;
using namespace NTextMachine::NCore;

class TBagOfWordsTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TBagOfWordsTest);
        UNIT_TEST(TestBagOfWordsContainter);
        //UNIT_TEST(TestBagOfWordsAccumulator);
        UNIT_TEST(TestBitMaskAccumulator);
        //UNIT_TEST(TestBagFractionAccumulator);
    UNIT_TEST_SUITE_END();

    TMemoryPool Pool{32 << 10};

public:

    TBagOfWords* CreateBagOfWordsContainer() {
        TBagOfWords* container = Pool.New<TBagOfWords>(Pool, 300, 50, 50);

        TWeightsHolder mainFormWeights1(Pool, 3, EStorageMode::Empty);
        mainFormWeights1.Add(3.0f);
        mainFormWeights1.Add(2.0f);
        mainFormWeights1.Add(5.0f);
        TWeightsHolder exactWeights1(Pool, 3, EStorageMode::Empty);
        exactWeights1.Add(2.0f);
        exactWeights1.Add(2.0f);
        exactWeights1.Add(2.0f);
        container->AddQuery(5, 0.2f, mainFormWeights1, exactWeights1, false);

        TWeightsHolder mainFormWeights2(Pool, 1, EStorageMode::Empty);
        mainFormWeights2.Add(10.0f);
        TWeightsHolder exactWeights2(Pool, 1, EStorageMode::Empty);
        exactWeights2.Add(17.0f);
        container->AddQuery(10, 0.8f, mainFormWeights2, exactWeights2, true);

        TWeightsHolder mainFormWeights3(Pool, 4, EStorageMode::Empty);
        mainFormWeights3.Add(5.0f);
        mainFormWeights3.Add(5.0f);
        mainFormWeights3.Add(0.0f);
        mainFormWeights3.Add(0.0f);
        TWeightsHolder exactWeights3(Pool, 4, EStorageMode::Empty);
        exactWeights3.Add(0.0f);
        exactWeights3.Add(0.0f);
        exactWeights3.Add(0.0f);
        exactWeights3.Add(0.0f);
        container->AddQuery(100, 1.0f, mainFormWeights3, exactWeights3, false);
        container->Finish();

        return container;
    }

    void CompareBagWordWeights(const TWeightsHolder& expectedWeights, const TBagOfWords& container, TBagOfWords::EWordWeightType type) {
        UNIT_ASSERT_EQUAL(expectedWeights.Count(), container.GetWordCount());
        for (size_t i = 0; i < expectedWeights.Count(); ++i) {
            Cdbg << "Weight(" << i << ") = " << expectedWeights[i] << " = " << container.GetBagWordWeight(i, type) << Endl;
            UNIT_ASSERT_DOUBLES_EQUAL(expectedWeights[i], container.GetBagWordWeight(i, type), FloatEpsilon);
        }
    }

    float CalcBagSumWeight(const TBagOfWords& container, TBagOfWords::EWordWeightType type) {
        float result = 0.0f;
        for (size_t i : xrange(container.GetWordCount())) {
            result += container.GetBagWordWeight(i, type);
        }
        return result;
    }

    void TestBagOfWordsContainter() {
        auto container = CreateBagOfWordsContainer();

        UNIT_ASSERT_EQUAL(container->GetQueryCount(), 3);

        UNIT_ASSERT_EQUAL(container->GetOriginalQueryId(), 1);

        UNIT_ASSERT_EQUAL(container->GetWordCount(), 8);

        UNIT_ASSERT_EQUAL(container->GetQuerySize(0), 3);
        UNIT_ASSERT_EQUAL(container->GetQuerySize(1), 1);
        UNIT_ASSERT_EQUAL(container->GetQuerySize(2), 4);

        UNIT_ASSERT_DOUBLES_EQUAL(container->GetQueryWeight(0), 0.2f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetQueryWeight(1), 0.8f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetQueryWeight(2), 1.0f, FloatEpsilon);

        TQueryId bagQueryId;
        TQueryWordId bagWordId;
        container->GetBagIds(5, 1, bagQueryId, bagWordId);
        UNIT_ASSERT_EQUAL(bagQueryId, 0);
        UNIT_ASSERT_EQUAL(bagWordId, 1);
        container->GetBagIds(10, 0, bagQueryId, bagWordId);
        UNIT_ASSERT_EQUAL(bagQueryId, 1);
        UNIT_ASSERT_EQUAL(bagWordId, 3);
        container->GetBagIds(100, 0, bagQueryId, bagWordId);
        UNIT_ASSERT_EQUAL(bagQueryId, 2);
        UNIT_ASSERT_EQUAL(bagWordId, 4);
        container->GetBagIds(100, 3, bagQueryId, bagWordId);
        UNIT_ASSERT_EQUAL(bagQueryId, 2);
        UNIT_ASSERT_EQUAL(bagWordId, 7);

        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(0), 0.3f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(0, TBagOfWords::EWordWeightType::Exact), 1.0f / 3, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(3), 1.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(3, TBagOfWords::EWordWeightType::Exact), 1.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(4), 0.5f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(4, TBagOfWords::EWordWeightType::Exact), 0.25f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(7), 0.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(container->GetWordWeight(7, TBagOfWords::EWordWeightType::Exact), 0.25f, FloatEpsilon);

        TWeightsHolder expectedMainFormWeights(Pool, 8, EStorageMode::Empty);
        expectedMainFormWeights.Add(0.3f * 0.1f);
        expectedMainFormWeights.Add(0.2f * 0.1f);
        expectedMainFormWeights.Add(0.5f * 0.1f);
        expectedMainFormWeights.Add(1.0f * 0.4f);
        expectedMainFormWeights.Add(0.5f * 0.5f);
        expectedMainFormWeights.Add(0.5f * 0.5f);
        expectedMainFormWeights.Add(0.0f);
        expectedMainFormWeights.Add(0.0f);
        TWeightsHolder expectedExactWeights(Pool, 8, EStorageMode::Empty);
        expectedExactWeights.Add(1.0f / 3.0f * 0.1f);
        expectedExactWeights.Add(1.0f / 3.0f * 0.1f);
        expectedExactWeights.Add(1.0f / 3.0f * 0.1f);
        expectedExactWeights.Add(1.0f * 0.4f);
        expectedExactWeights.Add(0.25f * 0.5f);
        expectedExactWeights.Add(0.25f * 0.5f);
        expectedExactWeights.Add(0.25f * 0.5f);
        expectedExactWeights.Add(0.25f * 0.5f);
        UNIT_ASSERT_DOUBLES_EQUAL(CalcBagSumWeight(*container, TBagOfWords::EWordWeightType::Main), 1.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(CalcBagSumWeight(*container, TBagOfWords::EWordWeightType::Exact), 1.0f, FloatEpsilon);
        CompareBagWordWeights(expectedMainFormWeights, *container, TBagOfWords::EWordWeightType::Main);
        CompareBagWordWeights(expectedExactWeights, *container, TBagOfWords::EWordWeightType::Exact);
    }
/*
    void TestBagOfWordsAccumulator() {
        auto container = CreateBagOfWordsContainer();

        TBagOfWordsAccumulator accumulator;
        accumulator.Init(container.Get());
        accumulator.NewDoc();

        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.AnnotationMatchAcc.CalcMatch95AvgValue(), 0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxMatch(), 0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW2S2.CalcMaxMatch(), 0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.FullMatchAcc.CalcMaxPrediction(), 0.0f, FloatEpsilon);
        accumulator.StartAnnotation(0.3f, 3);
        accumulator.AddHit(5, 0, 0, 0, true);
        accumulator.AddHit(5, 1, 1, 1, true);
        accumulator.FinishAnnotation();
        float cosSimSqr = 0.5f * 2.0f / 3.0f;
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.AnnotationMatchAcc.CalcMatch95AvgValue(), 0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxMatch(), cosSimSqr * 0.2f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW2S2.CalcMaxMatch(), cosSimSqr * 0.2f * 0.2f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxPrediction(), cosSimSqr * 0.2f * 0.3f, FloatEpsilon);

        accumulator.StartAnnotation(0.4f, 1);
        accumulator.AddHit(100, 2, 0, 0, true);
        accumulator.FinishAnnotation();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.AnnotationMatchAcc.CalcMatch95AvgValue(), 0.4f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxMatch(), cosSimSqr * 0.2f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW2S2.CalcMaxMatch(), cosSimSqr * 0.2f * 0.2f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxPrediction(), cosSimSqr * 0.2f * 0.3f, FloatEpsilon);

        accumulator.StartAnnotation(0.9f, 2);
        accumulator.AddHit(10, 0, 1, 1, true);
        accumulator.FinishAnnotation();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.AnnotationMatchAcc.CalcMatch95AvgValue(), 0.4f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxMatch(), 0.5f * 0.8f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW2S2.CalcMaxMatch(), 0.5f * 0.8f * 0.8f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxPrediction(), 0.5f * 0.8f * 0.9f, FloatEpsilon);

        accumulator.StartAnnotation(0.7f, 10);
        accumulator.AddHit(100, 3, 9, 9, true);
        accumulator.AddHit(100, 2, 8, 8, true);
        accumulator.AddHit(100, 1, 1, 7, true);
        accumulator.AddHit(100, 0, 0, 0, true);
        accumulator.FinishAnnotation();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.AnnotationMatchAcc.CalcMatch95AvgValue(), (0.4f + 0.7f) / 2.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxMatch(), 1.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW2S2.CalcMaxMatch(), 1.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxPrediction(), 0.7f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.FullMatchAcc.CalcMaxPrediction(), 0.0f, FloatEpsilon);

        accumulator.StartAnnotation(0.1f, 3);
        accumulator.AddHit(5, 0, 0, 0, true);
        accumulator.AddHit(5, 1, 1, 2, true);
        accumulator.AddHit(5, 2, 2, 2, true);
        accumulator.FinishAnnotation();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.AnnotationMatchAcc.CalcMatch95AvgValue(), (0.4f + 0.7f + 0.1f) / 3.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxMatch(), 1.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW2S2.CalcMaxMatch(), 1.0f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CosineSimilarityAccW1S2.CalcMaxPrediction(), 0.7f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.FullMatchAcc.CalcMaxPrediction(), 0.1f * 0.2f, FloatEpsilon);
    }
*/
    void TestBitMaskAccumulator() {
        TBitMaskAccumulator bitMaskAccumulator;
        bitMaskAccumulator.Update(33);
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 1);
        bitMaskAccumulator.Update(1);
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 1);

        bitMaskAccumulator.Clear();
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 0);
        bitMaskAccumulator.Update(1);
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 1);
        bitMaskAccumulator.Update(33);
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 1);
        bitMaskAccumulator.Update(2);
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 2);

        bitMaskAccumulator.Clear();
        for (size_t i = 5; i < 5 + 32; ++i) {
            bitMaskAccumulator.Update(i);
        }
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 32);
        for (size_t i = 25; i < 25 + 32; ++i) {
            bitMaskAccumulator.Update(i);
        }
        UNIT_ASSERT_EQUAL(bitMaskAccumulator.GetCount(), 32);
    }

/*
    void TestBagFractionAccumulator() {
        auto container = new CreateBagOfWordsContainer();
        TWeightsHolder weights = ContOf<float>(0.3f * 0.1f)(0.2f * 0.1f)(0.5f * 0.1f) (1.0f * 0.4f) (0.5f * 0.5f)(0.5f * 0.5f)(0.0f)(0.0f);

        TBagFractionAccumulator accumulator;
        accumulator.Init(container.Get());

        accumulator.NewDoc();
        accumulator.AddHit(TStream::Title, 3, 0, 1, 3);
        accumulator.AddHit(TStream::Title, 3, 0, 0, 0);

        accumulator.AddHit(TStream::Body, 3, 0, 0, 1);
        accumulator.AddHit(TStream::Body, 3, 0, 0, 2);

        accumulator.AddHit(TStream::Title, 3, 1, 0, 1);

        accumulator.AddHit(TStream::Title, 4, 1, 1, 3);

        accumulator.AddHit(TStream::Title, 10, 2, 2, 4);
        accumulator.AddHit(TStream::Title, 10, 2, 1, 3);
        accumulator.FinishDoc();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CalcOriginalRequestBagFraction(), weights[0] + weights[3] + weights[4], FloatEpsilon);


        accumulator.NewDoc();
        accumulator.FinishDoc();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CalcOriginalRequestBagFraction(), 0, FloatEpsilon);

        accumulator.NewDoc();
        accumulator.AddHit(TStream::Title, 0, 0, 1, 3);
        accumulator.AddHit(TStream::Body,  0, 0, 0, 0);
        accumulator.AddHit(TStream::Body,  0, 0, 0, 1);
        accumulator.AddHit(TStream::Body,  0, 0, 0, 2);
        accumulator.AddHit(TStream::Body,  0, 0, 2, 4);
        accumulator.AddHit(TStream::Body,  0, 0, 2, 5);
        accumulator.AddHit(TStream::Body,  0, 0, 2, 6);
        accumulator.AddHit(TStream::Body,  0, 0, 2, 7);
        accumulator.FinishDoc();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CalcOriginalRequestBagFraction(), weights[3], FloatEpsilon);

        accumulator.NewDoc();
        accumulator.AddHit(TStream::Body, 0, 0, 1, 3);
        accumulator.AddHit(TStream::Body, 0, 0, 0, 0);
        accumulator.AddHit(TStream::Body, 0, 0, 0, 1);
        accumulator.AddHit(TStream::Body, 0, 0, 0, 2);
        accumulator.AddHit(TStream::Body, 0, 0, 2, 4);
        accumulator.AddHit(TStream::Body, 0, 0, 2, 5);
        accumulator.AddHit(TStream::Body, 0, 0, 2, 6);
        accumulator.AddHit(TStream::Body, 0, 0, 2, 7);
        accumulator.FinishDoc();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CalcOriginalRequestBagFraction(), 1.0f, FloatEpsilon);

        accumulator.NewDoc();
        accumulator.AddHit(TStream::Body, 0, 0, 1, 3);

        accumulator.AddHit(TStream::Title, 0, 0, 1, 3);
        accumulator.AddHit(TStream::Title, 0, 0, 0, 0);
        accumulator.AddHit(TStream::Title, 0, 0, 0, 1);

        accumulator.AddHit(TStream::Title, 1, 1, 1, 3);

        accumulator.AddHit(TStream::Title, 1, 2, 2, 5);
        accumulator.AddHit(TStream::Title, 1, 2, 2, 6);

        accumulator.AddHit(TStream::Title, 3, 3, 1, 3);

        accumulator.AddHit(TStream::Title, 4, 3, 2, 5);
        accumulator.AddHit(TStream::Title, 4, 3, 2, 6);

        accumulator.AddHit(TStream::Title, 4, 4, 2, 5);
        accumulator.AddHit(TStream::Title, 4, 4, 2, 6);
        accumulator.AddHit(TStream::Title, 4, 4, 2, 7);

        accumulator.AddHit(TStream::Body, 4, 4, 1, 3);
        accumulator.AddHit(TStream::Body, 4, 4, 0, 2);
        accumulator.AddHit(TStream::Body, 4, 4, 2, 4);
        accumulator.FinishDoc();
        UNIT_ASSERT_DOUBLES_EQUAL(accumulator.CalcOriginalRequestBagFraction(), weights[0] + weights[1] + weights[2] + weights[3] + weights[4], FloatEpsilon);
    }
*/
};

UNIT_TEST_SUITE_REGISTRATION(TBagOfWordsTest);
