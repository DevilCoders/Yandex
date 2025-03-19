#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/codecs/float_huffman.h>

#include <util/generic/utility.h>

#include "factor_storage.h"
#include "factors_reader.h"

using namespace NFSSaveLoad;
using namespace NFactorSlices;

struct TTestData {
    const TVector<float> rawFactors = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f};
    const TVector<float> rawFactorsSkipMiddle = {0.0f, 0.1f, 0.2f, 0.6f, 0.7f, 0.8f, 0.9f};
    const TVector<float> rawFactorsTruncMiddle = {0.0f, 0.1f, 0.2f, 0.3f, 0.6f, 0.7f, 0.8f, 0.9f};
    const TVector<float> rawFactorsSkipMargins = {0.3f, 0.4f, 0.5f};
    const TVector<float> rawFactorsAddMetaRearrange = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.0f, 0.6f, 0.7f, 0.8f, 0.9f};

    TVector<float> largeFactors = TVector<float>(1244, 1.0f);

    const char* bordersStr = "web_production[0;3) web_meta[3;6) personalization[6;10)";
    const char* bordersStrSkipMeta = "web_production[0;3) personalization[3;7)";
    const char* bordersStrSkipProdAndPers = "web_meta[0;3)";
    const char* bordersStrTruncMeta = "web_production[0;3) web_meta[3;4) personalization[4;8)";
    const char* bordersStrAddMetaRearrange = "web_production[0;3) web_meta[3;6) web_meta_rearrange[6;7) personalization[7;11)";
    const char* largeBordersStr = "web_production[0;1091) web_meta[1091;1175) web_meta_rearrange[1175;1176)"
        " rapid_clicks[1176;1212) personalization[1212;1244)";
};

class TRawInputBase
    : public TTestData
{
public:
    THolder<TFloatsInput> CreateInput() {
        return CreateRawFloatsInput(rawFactors.data(), rawFactors.size());
    }

    THolder<TFloatsInput> CreateLargeInput() {
        return CreateRawFloatsInput(largeFactors.data(), largeFactors.size());
    }
};

class THuffmanInputBase
    : public TTestData
{
public:
    TString compressedFactors, largeCompressedFactors;

    THolder<TFloatsInput> CreateInput() {
        if (compressedFactors.empty()) {
            compressedFactors = NCodecs::NFloatHuff::Encode(rawFactors);
        }

        return CreateHuffmanFloatsInput(compressedFactors);
    }

    THolder<TFloatsInput> CreateLargeInput() {
        if (largeCompressedFactors.empty()) {
            largeCompressedFactors = NCodecs::NFloatHuff::Encode(largeFactors);
        }

        return CreateHuffmanFloatsInput(largeCompressedFactors);
    }

};

template <typename BaseType>
class TGenericReaderTest
    : public BaseType
{
public:
    using BaseType::CreateInput;
    using BaseType::CreateLargeInput;

    THolder<TSkipFactorsInput> CreateSkipInput(const TFactorsSkipSet& skipSet = TFactorsSkipSet()) {
        return MakeHolder<TSkipFactorsInput>(CreateInput(), skipSet);
    }

    THolder<TFactorsReader> CreateReader()
    {
         return NFSSaveLoad::CreateReader(BaseType::bordersStr, CreateInput());
    }

    template <typename InputType>
    void CheckInput(InputType&& input, const TVector<float>& expectedRes) {
        TVector<float> res;
        res.resize(expectedRes.size());

        const size_t loaded = input->Load(res.data(), res.size());
        Cdbg << "SIZE: " << loaded << " == " << expectedRes.size() << Endl;
        UNIT_ASSERT_EQUAL(loaded, expectedRes.size());
        UNIT_ASSERT_EQUAL(input->GetLoadedCount(), loaded);
        for (size_t i : xrange(expectedRes.size())) {
            Cdbg << "VALUE(" << i << "): " << res[i] << " == " << expectedRes[i] << Endl;
            UNIT_ASSERT_EQUAL(res[i], expectedRes[i]);
        }
    }

    template <typename ReaderType>
    void CheckReadToVector(ReaderType&& reader, const TVector<float>& expectedRes) {
        TVector<float> res;
        reader->ReadTo(res);

        Cdbg << "SIZE: " << res.size() << " == " << expectedRes.size() << Endl;
        UNIT_ASSERT_EQUAL(res.size(), expectedRes.size());
        for (size_t i : xrange(expectedRes.size())) {
            Cdbg << "VALUE(" << i << "): " << res[i] << " == " << expectedRes[i] << Endl;
            UNIT_ASSERT_EQUAL(res[i], expectedRes[i]);
        }
    }

    template <typename ReaderType>
    void CheckReadToStorage(ReaderType&& reader, const TVector<float>& expectedRes,
        const TStringBuf& expectedBordersStr)
    {
        TFactorStorage res;
        reader->ReadTo(res);

        Cdbg << "SIZE: " << res.Size() << " == " << expectedRes.size() << Endl;
        UNIT_ASSERT_EQUAL(res.Size(), expectedRes.size());
        for (size_t i : xrange(expectedRes.size())) {
            Cdbg << "VALUE(" << i << "): " << res[i] << " == " << expectedRes[i] << Endl;
            UNIT_ASSERT_EQUAL(res[i], expectedRes[i]);
        }

        const TString bordersStr = SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly);
        Cdbg << "BORDERS: " << bordersStr << " == " << expectedBordersStr << Endl;
        UNIT_ASSERT_EQUAL(bordersStr, expectedBordersStr);
    }


    template <typename ReaderType>
    void CheckReadToHostStorage(ReaderType&& reader, const TVector<float>& expectedRes,
        const TStringBuf& hostBordersStr, TFactorsReader::EHostMode mode, const TStringBuf& expectedBordersStr)
    {
        TFactorBorders hostBorders;
        DeserializeFactorBorders(hostBordersStr, hostBorders);
        TSlicesMetaInfo hostInfo;
        UNIT_ASSERT(NFactorSlices::NDetail::ReConstructMetaInfo(hostBorders, hostInfo));

        TFactorStorage res;
        reader->ReadTo(res, hostInfo, mode);

        Cdbg << "SIZE: " << res.Size() << " == " << expectedRes.size() << Endl;
        UNIT_ASSERT_EQUAL(res.Size(), expectedRes.size());
        for (size_t i : xrange(expectedRes.size())) {
            Cdbg << "VALUE(" << i << "): " << res[i] << " == " << expectedRes[i] << Endl;
            UNIT_ASSERT_EQUAL(res[i], expectedRes[i]);
        }

        const TString bordersStr = SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly);
        Cdbg << "BORDERS: " << bordersStr << " == " << expectedBordersStr << Endl;
        UNIT_ASSERT_EQUAL(bordersStr, expectedBordersStr);
    }

    void DoTestSimpleCopy() {
        CheckInput(CreateInput(), BaseType::rawFactors);
    }

    void DoTestTwoStepCopy() {
        auto input = CreateInput();
        TVector<float> res;
        res.resize(BaseType::rawFactors.size());
        UNIT_ASSERT_EQUAL(input->Load(res.data(), BaseType::rawFactors.size() / 2), BaseType::rawFactors.size() / 2);
        UNIT_ASSERT_EQUAL(input->Load(res.data() + BaseType::rawFactors.size() / 2, BaseType::rawFactors.size() - BaseType::rawFactors.size() / 2), BaseType::rawFactors.size() - BaseType::rawFactors.size() / 2);

        for (size_t i : xrange(BaseType::rawFactors.size())) {
            UNIT_ASSERT_EQUAL(res[i], BaseType::rawFactors[i]);
        }
    }

    void DoTestZeroLoad() {
        auto input = CreateInput();
        float buf;
        UNIT_ASSERT_EQUAL(input->Load(&buf, 0), 0);
    }

    void DoTestExcessiveLoad() {
        auto input = CreateInput();
        TVector<float> res;
        res.resize(BaseType::rawFactors.size() * 2);
        UNIT_ASSERT_EQUAL(input->Load(res.data(), res.size()), BaseType::rawFactors.size());
    }

    void DoTestNoSkip() {
        CheckInput(CreateSkipInput({}), BaseType::rawFactors);
    }

    void DoTestEmptySkips() {
        CheckInput(CreateSkipInput({TSliceOffsets(0, 0), TSliceOffsets(1, 1), TSliceOffsets(2, 2), TSliceOffsets(3, 3)}), BaseType::rawFactors);
    }

    void DoTestSkipMiddle() {
        CheckInput(CreateSkipInput({TSliceOffsets(3, 6)}), BaseType::rawFactorsSkipMiddle);
    }

    void DoTestSkipMargins() {
        CheckInput(CreateSkipInput({TSliceOffsets(0, 3), TSliceOffsets(6, 10)}), BaseType::rawFactorsSkipMargins);
    }

    void DoTestSkipOverlap() {
        CheckInput(CreateSkipInput({TSliceOffsets(3, 5), TSliceOffsets(4, 6)}), BaseType::rawFactorsSkipMiddle);
    }

    void DoTestSkipTwice() {
        CheckInput(CreateSkipInput({TSliceOffsets(3, 6), TSliceOffsets(3, 6)}), BaseType::rawFactorsSkipMiddle);
    }

    void DoTestSkipAll() {
        CheckInput(CreateSkipInput({TSliceOffsets(0, 10)}), {});
    }

    void DoTestRemoveMiddleSlice() {
        {
            auto reader = CreateReader();
            reader->RemoveSlice(EFactorSlice::WEB_META);
            CheckReadToVector(reader, BaseType::rawFactorsSkipMiddle);
        }
        {
            auto reader = CreateReader();
            reader->RemoveSlice(EFactorSlice::WEB_META);
            CheckReadToStorage(reader, BaseType::rawFactorsSkipMiddle, BaseType::bordersStrSkipMeta);
        }
    }

    void DoTestRemoveMarginSlices() {
        auto reader = CreateReader();
        reader->RemoveSlice(EFactorSlice::WEB_PRODUCTION);
        reader->RemoveSlice(EFactorSlice::PERSONALIZATION);
        CheckReadToStorage(reader, BaseType::rawFactorsSkipMargins, BaseType::bordersStrSkipProdAndPers);
    }

    void DoTestTruncMiddleSlice() {
        auto reader = CreateReader();
        CheckReadToHostStorage(reader, BaseType::rawFactorsTruncMiddle,
            BaseType::bordersStrTruncMeta, TFactorsReader::EHostMode::PadAndTruncate,
            BaseType::bordersStrTruncMeta);
    }

    void DoTestAddMetaRearrange() {
        auto reader = CreateReader();
        CheckReadToHostStorage(reader, BaseType::rawFactorsAddMetaRearrange,
            BaseType::bordersStrAddMetaRearrange, TFactorsReader::EHostMode::PadOnly,
            BaseType::bordersStrAddMetaRearrange);
    }

    void DoTestLargeRemoveAllButOneSlices() {
        {
            auto reader = NFSSaveLoad::CreateReader(BaseType::largeBordersStr, CreateLargeInput());
            for (TLeafIterator iter; iter.Valid(); iter.Next()) {
                if (*iter != EFactorSlice::PERSONALIZATION) {
                    reader->RemoveSlice(*iter);
                }
            }
            TFactorStorage res;
            reader->ReadTo(res);
            Cdbg << "BORDERS: " << SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly)
                << " == " << "personalization[0;32)" << Endl;
            UNIT_ASSERT_EQUAL(SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly), "personalization[0;32)");
        }
    }

    void DoTestMoveSliceInPlace() {
        {
            TFactorStorage res;
            UNIT_ASSERT(MoveSlice(res, EFactorSlice::WEB_PRODUCTION, EFactorSlice::VIDEO_PRODUCTION));
        }

        {
            auto reader = NFSSaveLoad::CreateReader(BaseType::bordersStr, CreateInput());
            TFactorStorage res;
            reader->ReadTo(res);

            UNIT_ASSERT(MoveSlice(res, EFactorSlice::WEB_PRODUCTION, EFactorSlice::WEB_PRODUCTION));

            for (size_t i : xrange(res.Size())) {
                UNIT_ASSERT_EQUAL(res[i], float(i) / 10.f);
            }

            UNIT_ASSERT(MoveSlice(res, EFactorSlice::WEB_PRODUCTION, EFactorSlice::VIDEO_PRODUCTION));

            Cdbg << "BORDERS: " << SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly) << Endl;
            UNIT_ASSERT_EQUAL(SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly), "web_meta[0;3) personalization[3;7) video_production[7;10)");

            auto view = res.CreateViewFor(EFactorSlice::WEB_PRODUCTION);
            UNIT_ASSERT_EQUAL(view.Size(), 0);

            view = res.CreateViewFor(EFactorSlice::VIDEO_PRODUCTION);
            UNIT_ASSERT_EQUAL(view.Size(), 3);
            UNIT_ASSERT_EQUAL(view[0], 0.0f);
            UNIT_ASSERT_EQUAL(view[1], 0.1f);
            UNIT_ASSERT_EQUAL(view[2], 0.2f);

            view = res.CreateViewFor(EFactorSlice::WEB_META);
            UNIT_ASSERT_EQUAL(view.Size(), 3);
            UNIT_ASSERT_EQUAL(view[0], 0.3f);
            UNIT_ASSERT_EQUAL(view[1], 0.4f);
            UNIT_ASSERT_EQUAL(view[2], 0.5f);

            view = res.CreateViewFor(EFactorSlice::PERSONALIZATION);
            UNIT_ASSERT_EQUAL(view.Size(), 4);
            UNIT_ASSERT_EQUAL(view[0], 0.6f);
            UNIT_ASSERT_EQUAL(view[1], 0.7f);
            UNIT_ASSERT_EQUAL(view[2], 0.8f);
            UNIT_ASSERT_EQUAL(view[3], 0.9f);

            MoveSlice(res, EFactorSlice::VIDEO_PRODUCTION, EFactorSlice::WEB_PRODUCTION);

            Cdbg << "BORDERS: " << SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly) << Endl;
            UNIT_ASSERT_EQUAL(SerializeFactorBorders(res.GetBorders(), ESerializationMode::LeafOnly), "web_production[0;3) web_meta[3;6) personalization[6;10)");

            for (size_t i : xrange(res.Size())) {
                UNIT_ASSERT_EQUAL(res[i], float(i) / 10.f);
            }
        }
    }
};

Y_UNIT_TEST_SUITE(RawFactorsReaderTest) {
    TGenericReaderTest<TRawInputBase> rawTest;

    Y_UNIT_TEST(TestSimpleCopy) {
        rawTest.DoTestSimpleCopy();
    }

    Y_UNIT_TEST(TestTwoStepCopy) {
        rawTest.DoTestTwoStepCopy();
    }

    Y_UNIT_TEST(TestZeroLoad) {
        rawTest.DoTestZeroLoad();
    }

    Y_UNIT_TEST(TestExcessiveLoad) {
        rawTest.DoTestExcessiveLoad();
    }

    Y_UNIT_TEST(TestNoSkip) {
        rawTest.DoTestNoSkip();
    }

    Y_UNIT_TEST(TestEmptySkips) {
        rawTest.DoTestEmptySkips();
    }

    Y_UNIT_TEST(TestSkipMiddle) {
        rawTest.DoTestSkipMiddle();
    }

    Y_UNIT_TEST(TestTruncMiddle) {
        rawTest.DoTestTruncMiddleSlice();
    }

    Y_UNIT_TEST(TestSkipMargins) {
        rawTest.DoTestSkipMargins();
    }

    Y_UNIT_TEST(TestSkipOverlap) {
        rawTest.DoTestSkipOverlap();
    }

    Y_UNIT_TEST(TestSkipTwice) {
        rawTest.DoTestSkipTwice();
    }

    Y_UNIT_TEST(TestSkipAll) {
        rawTest.DoTestSkipAll();
    }

    Y_UNIT_TEST(TestRemoveMiddleSlice) {
        rawTest.DoTestRemoveMiddleSlice();
    }

    Y_UNIT_TEST(TestRemoveMarginSlices) {
        rawTest.DoTestRemoveMarginSlices();
    }

    Y_UNIT_TEST(TestAddMetaRearrange) {
        rawTest.DoTestAddMetaRearrange();
    }

    Y_UNIT_TEST(TestLargeRemoveAllButOneSlices) {
        rawTest.DoTestLargeRemoveAllButOneSlices();
    }

    Y_UNIT_TEST(TestMoveSliceInPlace) {
        rawTest.DoTestMoveSliceInPlace();
    }

    Y_UNIT_TEST(ProblemSlicesReadingCase) {
        TVector<float> res = TransformFeaturesVectorToSlices(
            {0.f + 1.f, 1.f + 1.f, 2.f + 1.f, 3.f + 1.f, 4.f + 1.f},
            "web_production[0;2) web_production_formula_features[2;3) rapid_clicks[3;4) rapid_pers_clicks[4;5)",
            "web_production[0;1) web_production_formula_features[1;3) rapid_clicks[3;5)"
        );
        UNIT_ASSERT_VALUES_EQUAL(res[0], 0.f + 1.f);
        UNIT_ASSERT_VALUES_EQUAL(res[1], 2.f + 1.f);
        UNIT_ASSERT_VALUES_EQUAL(res[2], 0.f);
        UNIT_ASSERT_VALUES_EQUAL(res[3], 3.f + 1.f);
        UNIT_ASSERT_VALUES_EQUAL(res[4], 0.f);
    }

    Y_UNIT_TEST(ProblemSlicesReadingCase2) {
        TVector<float> res = TransformFeaturesVectorToSlices(
            {0.f + 1.f, 1.f + 1.f, 2.f + 1.f, 3.f + 1.f, 4.f + 1.f, 5.f + 1.f},
            "web_production[0;1) web_meta[1;2) web_meta_pers[2;3) web_rtmodels[3;4) rapid_clicks[4;5) rapid_clicks_l2[5;6)",
            "web_production[0;1) web_meta_pers[1;3) web_rtmodels[3;4) rapid_clicks[4;5)"
        );
        UNIT_ASSERT_VALUES_EQUAL(res[0], 0.f + 1.f);
        UNIT_ASSERT_VALUES_EQUAL(res[1], 2.f + 1.f);
        UNIT_ASSERT_VALUES_EQUAL(res[2], 0.f);
        UNIT_ASSERT_VALUES_EQUAL(res[3], 3.f + 1.f);
        UNIT_ASSERT_VALUES_EQUAL(res[4], 4.f + 1.f);
    }
};

Y_UNIT_TEST_SUITE(HuffmanFactorsReaderTest) {
    TGenericReaderTest<THuffmanInputBase> huffmanTest;

    Y_UNIT_TEST(TestSimpleCopy) {
        huffmanTest.DoTestSimpleCopy();
    }

    Y_UNIT_TEST(TestTwoStepCopy) {
        huffmanTest.DoTestTwoStepCopy();
    }

    Y_UNIT_TEST(TestZeroLoad) {
        huffmanTest.DoTestZeroLoad();
    }

    Y_UNIT_TEST(TestExcessiveLoad) {
        huffmanTest.DoTestExcessiveLoad();
    }

    Y_UNIT_TEST(TestNoSkip) {
        huffmanTest.DoTestNoSkip();
    }

    Y_UNIT_TEST(TestEmptySkips) {
        huffmanTest.DoTestEmptySkips();
    }

    Y_UNIT_TEST(TestSkipMiddle) {
        huffmanTest.DoTestSkipMiddle();
    }

    Y_UNIT_TEST(TestTruncMiddle) {
        huffmanTest.DoTestTruncMiddleSlice();
    }

    Y_UNIT_TEST(TestSkipMargins) {
        huffmanTest.DoTestSkipMargins();
    }

    Y_UNIT_TEST(TestSkipOverlap) {
        huffmanTest.DoTestSkipOverlap();
    }

    Y_UNIT_TEST(TestSkipTwice) {
        huffmanTest.DoTestSkipTwice();
    }

    Y_UNIT_TEST(TestSkipAll) {
        huffmanTest.DoTestSkipAll();
    }

    Y_UNIT_TEST(TestRemoveMiddleSlice) {
        huffmanTest.DoTestRemoveMiddleSlice();
    }

    Y_UNIT_TEST(TestRemoveMarginSlices) {
        huffmanTest.DoTestRemoveMarginSlices();
    }

    Y_UNIT_TEST(TestAddMetaRearrange) {
        huffmanTest.DoTestAddMetaRearrange();
    }

    Y_UNIT_TEST(TestLargeRemoveAllButOneSlices) {
        huffmanTest.DoTestLargeRemoveAllButOneSlices();
    }

    Y_UNIT_TEST(TestMoveSliceInPlace) {
        huffmanTest.DoTestMoveSliceInPlace();
    }
};
