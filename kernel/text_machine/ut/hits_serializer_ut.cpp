#include <kernel/text_machine/util/hits_serializer.h>
#include <kernel/text_machine/util/misc.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTextMachine;

class TSerializerTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TSerializerTest);
        UNIT_TEST(TestEmptySerializerV0);
        UNIT_TEST(TestEmptySerializerV1);
        UNIT_TEST(TestSerializerSaveLoadV0);
        UNIT_TEST(TestSerializerSaveLoadV1);
        UNIT_TEST(TestSerializerSaveLoadV1OneStream);
    UNIT_TEST_SUITE_END();

private:
    template <typename HitType, typename AddFuncType, typename NextFuncType>
    void VerifyHitsSaveLoad(
        const HitType* hits,
        const size_t hitsNum,
        const AddFuncType& addFunc,
        const NextFuncType& nextFunc,
        bool useCompression)
    {
        NTextMachineProtocol::TPbHits hitsProto;
        THitsSerializer::TOptions serOptions;
        serOptions.UseOffroadCompression = useCompression;
        THitsSerializer ser(&hitsProto, serOptions);

        for (size_t i = 0; i != hitsNum; ++i) {
            addFunc(ser, hits[i]);
        }

        ser.Finish();

        THitsDeserializer deser(&hitsProto);
        deser.Init();
        HitType curHit;
        size_t curIndex = 0;

        while (nextFunc(deser, curHit)) {
            // Cerr << curIndex << '\t' << hits[curIndex] << Endl;
            // Cerr << curIndex << '\t' << curHit << Endl;

            UNIT_ASSERT(curIndex < hitsNum);
            UNIT_ASSERT(hits[curIndex] == curHit);
            curIndex += 1;
        }

        UNIT_ASSERT(curIndex == hitsNum);
    }

public:
    void DoTestEmptySerializer(bool useCompression) {
        NTextMachineProtocol::TPbHits hitsProto;
        THitsSerializer::TOptions serOptions;
        serOptions.UseOffroadCompression = useCompression;
        THitsSerializer ser(&hitsProto, serOptions);
        // Do nothing
        ser.Finish();
        THitsDeserializer deser(&hitsProto);

        THit wordHit;
        TBlockHit blockHit;

        UNIT_CHECK_GENERATED_EXCEPTION(deser.NextWordHit(wordHit), yexception);
        UNIT_CHECK_GENERATED_EXCEPTION(deser.NextBlockHit(blockHit), yexception);

        deser.Init();

        UNIT_ASSERT(!deser.NextWordHit(wordHit));
        UNIT_ASSERT(!deser.NextBlockHit(blockHit));
    }

    void TestEmptySerializerV0() {
        DoTestEmptySerializer(false);
    }

    void TestEmptySerializerV1() {
        DoTestEmptySerializer(true);
    }

    void DoTestSerializerSaveLoad(bool useCompression, bool onlyOneStream) {
        TStream titleStream(TStream::Title);
        titleStream.AnnotationCount = 1;
        titleStream.WordCount = 10;
        titleStream.MaxValue = 0.7;

        TStream wikiStream(TStream::WikiTitle);
        wikiStream.AnnotationCount = 5;
        wikiStream.WordCount = 80;
        wikiStream.MaxValue = 0.8;

        const TStream streams[] = { titleStream, wikiStream};

        const TAnnotation titAnn[] = {
            {&streams[0], 0, 0, 0, 10, 0.7f}
        };

        const TAnnotation wikiAnn[] = {
            {&streams[1], 0, 2, 20, 10, 0.4f},
            {&streams[1], 0, 4, 60, 10, 0.75f}
        };

        const TPosition titPos[] = {
            {&titAnn[0], 3, 3},
            {&titAnn[0], 5, 8}
        };

        const TPosition wikiPos[] = {
            {&wikiAnn[0], 4, 4},
            {&wikiAnn[1], 8, 9}
        };

        const TBlockHit blockHits[] = {
            {TBaseIndexLayer::Layer0, titPos[0], 3, TMatchPrecision::Exact, 0, 0.0f, 0, 0},
            {TBaseIndexLayer::Layer0, titPos[1], 4, TMatchPrecision::Unknown, 0, 0.0f, 0, 0},
            {TBaseIndexLayer::Layer0, wikiPos[0], 5, TMatchPrecision::Lemma, 2, 0.0f, 0, 0},
            {TBaseIndexLayer::Layer0, wikiPos[1], 4, TMatchPrecision::Unknown, 0, 0.0f, 0, 0}
        };

        VerifyHitsSaveLoad(blockHits, onlyOneStream ? 2 : Y_ARRAY_SIZE(blockHits),
            [](THitsSerializer& ser, const TBlockHit& hit) {ser.AddBlockHit(hit);},
            [](THitsDeserializer& deser, TBlockHit& hit) -> bool {return deser.NextBlockHit(hit);},
            useCompression);

        const THit wordHits[] = {
            {titPos[0], {0, 0, 0}, 0.0},
            {titPos[1], {0, 1, 0}, 0.0},
            {wikiPos[0], {1, 0, 1}, 0.0},
            {wikiPos[1], {1, 1, 1}, 0.0}
        };

        VerifyHitsSaveLoad(wordHits, onlyOneStream ? 2 : Y_ARRAY_SIZE(wordHits),
            [](THitsSerializer& ser, const THit& hit) {ser.AddWordHit(hit);},
            [](THitsDeserializer& deser, THit& hit) -> bool {return deser.NextWordHit(hit);},
            useCompression);

        const TBlockHit badBlockHits[] = {
            {TBaseIndexLayer::Layer0, wikiPos[1], 4, TMatchPrecision::Unknown, 0, 0.0, 0, 0},
            {TBaseIndexLayer::Layer0, wikiPos[0], 5, TMatchPrecision::Lemma, 2, 0.0, 0, 0}
        };

        UNIT_CHECK_GENERATED_EXCEPTION(
            VerifyHitsSaveLoad(badBlockHits, Y_ARRAY_SIZE(badBlockHits),
                [](THitsSerializer& ser, const TBlockHit& hit) {ser.AddBlockHit(hit);},
                [](THitsDeserializer& deser, TBlockHit& hit) -> bool {return deser.NextBlockHit(hit);},
                useCompression),
            yexception
        );
    }

    void TestSerializerSaveLoadV0() {
        DoTestSerializerSaveLoad(false, false);
    }

    void TestSerializerSaveLoadV1() {
        DoTestSerializerSaveLoad(true, false);
    }

    void TestSerializerSaveLoadV1OneStream() {
        DoTestSerializerSaveLoad(true, true);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TSerializerTest);
