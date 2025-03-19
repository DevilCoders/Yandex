#include <kernel/text_machine/interface/text_machine.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTextMachine;

class TFeatureIdTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TFeatureIdTest);
        UNIT_TEST(TestId);
    UNIT_TEST_SUITE_END();

public:
    void TestId() {
        TFFId f1(TAlgorithm::Bm11);
        UNIT_ASSERT_EQUAL(f1.Get<TFeaturePart::Algorithm>(), TAlgorithm::Bm11);

        TFFId f2(f1, TStream::Body);
        UNIT_ASSERT_EQUAL(f2.Get<TFeaturePart::Stream>(), TStream::Body);

        TFFId f3(f2, TExpansion::OriginalRequest);
        TFFId f4(f3, TNormalizer::Count, TAccumulator::SumF, TFilter::Tail);
        TFFId f5(TNormalizer::SumW, TAccumulator::MaxWF, TFilter::All, "BlaBlaBm11");
        TFFId f5_1("BlaBlaBm11", TFilter::All, TAccumulator::MaxWF, TNormalizer::SumW);

        UNIT_ASSERT_EQUAL(f4.Get<TFeaturePart::Stream>(), TStream::Body);
        UNIT_ASSERT_EQUAL(f5.Get<TFeaturePart::Filter>(), TFilter::All);

        UNIT_ASSERT_EQUAL(f1.FullName(), "Bm11");
        UNIT_ASSERT_EQUAL(f2.FullName(), "Body_Bm11");
        UNIT_ASSERT_EQUAL(f3.FullName(), "OriginalRequest_Body_Bm11");
        UNIT_ASSERT_EQUAL(f4.FullName(), "OriginalRequest_Tail_SumF_Count_Body_Bm11");
        UNIT_ASSERT_EQUAL(f5.FullName(), "All_MaxWF_SumW_BlaBlaBm11");
        UNIT_ASSERT_EQUAL(f5.FullName(), f5_1.FullName());

        UNIT_ASSERT_EQUAL(f5.SuffixName<TFeaturePart::Stream>(), "BlaBlaBm11");

        TFFId f6 = f5;

        UNIT_ASSERT_EQUAL(f5, f6);
        UNIT_ASSERT_EQUAL(f5.FullName(), f6.FullName());

        f6.Set(TStream::Body);
        UNIT_ASSERT_EQUAL(f6.FullName(), "All_MaxWF_SumW_Body_BlaBlaBm11");

        f6.Set(TStreamSet::FieldSet1);
        UNIT_ASSERT_EQUAL(f6.FullName(), "All_MaxWF_SumW_FieldSet1_BlaBlaBm11");

        f6.Set(TKValue(0.01));
        UNIT_ASSERT_EQUAL(f6.FullName(), "All_MaxWF_SumW_FieldSet1_K0.01_BlaBlaBm11");

        TFFId f7 = f6;
        f7.UnSet<TFeaturePart::Filter>();
        UNIT_ASSERT_EQUAL(f7.FullName(), "MaxWF_SumW_FieldSet1_K0.01_BlaBlaBm11");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TFeatureIdTest);
