#include <kernel/click_sim/binarized_word_hash_vector.h>
#include <kernel/click_sim/word_vector.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/vowpalwabbit/vowpal_wabbit_predictor.h>

#include <util/string/split.h>

using namespace NClickSim;

Y_UNIT_TEST_SUITE(TBinarizedWordHashVectorTest) {
    THashMap<TString , double> GetTestVector() {
        return {
            {"0", 0.030729941751039894},
            {"1", 0.08520766106264609},
            {"12", 0.04030132093618533},
            {"15", 0.02794591736678007},
            {"16", 0.04015178168758775},
            {"18", 0.006575960954898457},
            {"2", 0.07191531475703157},
            {"20", 0.016393149236462998},
            {"2003", 0.005421062541756163},
            {"22448", 0.013883161619849227},
            {"25", 0.010782512713934956},
            {"3", 0.020475164453101286},
            {"31", 0.006063346288271139},
            {"32", 0.028546658032160586},
            {"33", 0.013676837515368252},
            {"35", 0.0068970487191202965},
            {"4", 0.03389611551090097},
            {"5", 0.013210952564654365},
            {"50", 0.003877451077182117},
            {"6", 0.04050402710414308},
            {"8", 0.019179070890650393},
            {"90919", 0.010385054721188714},
            {"b", 0.008714621901345532},
            {"de", 0.016541464704864083},
            {"f", 0.008009902443337378},
            {"fb", 0.005680606425948219},
            {"j", 0.010546896636351086},
            {"n", 0.013525476582747865},
            {"nissan", 0.024642495195700092},
            {"qg", 0.014253364967345736},
            {"r", 0.004905719521333148},
            {"rb", 0.002830524938757309},
            {"teana", 0.0025600461898415013},
            {"vq", 0.0031797823565603467},
            {"z", 0.0025018047305591525},
            {"а", 0.03771173754122741},
            {"ад", 0.002236087060451982},
            {"альмера", 0.04246505397555626},
            {"благовещенске", 0.007670656013934154},
            {"блюберд", 0.0110110208878686},
            {"в", 0.11204856052955059},
            {"ваз", 0.01668161053416567},
            {"вингроуд", 0.00625887765075556},
            {"высоковольтные", 0.002059948230947253},
            {"г", 0.01189364307955429},
            {"года", 0.00974543131992596},
            {"двигатель", 0.004248425565943005},
            {"де", 0.0034293662967271386},
            {"для", 0.06528602820244989},
            {"е", 0.004546102745447084},
            {"зажигание", 0.0020131165480788936},
            {"зажигания", 0.7320488871111058},
            {"и", 0.0027863601363145754},
            {"изолятор", 0.032904006479141955},
            {"инфинити", 0.003783865310169579},
            {"к", 0.008249092887780451},
            {"катушек", 0.024624733533018885},
            {"катушка", 0.3700421317184028},
            {"катушки", 0.2525897019208984},
            {"катушку", 0.10201796025221553},
            {"кашкай", 0.01848794494707892},
            {"киа", 0.004444888768638522},
            {"клапанов", 0.011795396038835768},
            {"колпачки", 0.007072167325822793},
            {"конденсатор", 0.005652840705793857},
            {"королла", 0.0037787949252807854},
            {"куб", 0.007500376046736293},
            {"логан", 0.0037376452176776627},
            {"максима", 0.016401390598854204},
            {"модуль", 0.018909494796158235},
            {"н", 0.01350857648981933},
            {"на", 0.29762476719139974},
            {"наконечник", 0.06626763028233604},
            {"наконечники", 0.02697533251151842},
            {"ниссан", 0.32171006805791874},
            {"примера", 0.0409126514688804},
            {"провода", 0.009309684125226004},
            {"р", 0.030175325744498157},
            {"разъем", 0.006406870535709232},
            {"резинки", 0.00830550409531515},
            {"резиновые", 0.0030804616982183556},
            {"резиновый", 0.0022567057150526724},
            {"рено", 0.022491695371099733},
            {"с", 0.004423437709649463},
            {"санни", 0.03518873893184294},
            {"свечи", 0.023469237126926345},
            {"силфи", 0.00377419829932608},
            {"сколько", 0.013262658337238636},
            {"солярис", 0.004112058633828982},
            {"стартер", 0.002772365845372874},
            {"стоит", 0.012647154616764649},
            {"т", 0.003898156220154656},
            {"тиида", 0.0027582229397479724},
            {"тойота", 0.033172778107922615},
            {"трамблер", 0.002152328508494321},
            {"трамблера", 0.008557382589905483},
            {"фишка", 0.0017913874917525022},
            {"фишку", 0.0060963142233170136},
            {"цефиро", 0.018028118242152006},
            {"штекер", 0.003149463045644138}
        };
    }

    Y_UNIT_TEST(TestSaveLoad) {
        TBinarizedWordHashVector binarizedVector(GetTestVector());
        const TString s = binarizedVector.SaveToString();
        UNIT_ASSERT_EQUAL(s.size(), binarizedVector.Size() * sizeof(THashWithWeight));
        UNIT_ASSERT(memcmp(s.data(), binarizedVector.Data(), s.size()) == 0);

        TBinarizedWordHashVector restoredVector;
        restoredVector.Load(TArrayRef<const char>(s.data(), s.size()));
        UNIT_ASSERT_EQUAL(binarizedVector, restoredVector);
    }

    Y_UNIT_TEST(TestSaveLoadLeb128) {
        TBinarizedWordHashVector binarizedVector(GetTestVector());
        const TString s = binarizedVector.SaveToLeb128();
        TBinarizedWordHashVector restoredVector;
        restoredVector.LoadFromLeb128(s);
        UNIT_ASSERT_EQUAL(binarizedVector, restoredVector);
    }

    struct TTestCallback {
        size_t MatchedAccumulator = 0;
        size_t LeftZeroes = 0;
        size_t RightZeroes = 0;

        void operator() (const THashWithWeight* l, const THashWithWeight* r) {
            if (l == nullptr) {
                ++LeftZeroes;
            }

            if (r == nullptr) {
                ++RightZeroes;
            }

            if (l && r) {
                MatchedAccumulator += static_cast<ui32>(l->Weight) * r->Weight;
            }
        }

        void Clear() {
            MatchedAccumulator = 0;
            LeftZeroes = 0;
            RightZeroes = 0;
        }
    };

    Y_UNIT_TEST(TestDotProductWithCallback) {
        TMap<ui32, ui8> word2weight1;
        TBinarizedWordHashVector empty(word2weight1);
        word2weight1[1] = 1;
        word2weight1[2] = 2;
        word2weight1[4] = 4;

        TBinarizedWordHashVector v1(word2weight1);
        TTestCallback cb;

        v1.DotProductWithCallback<false>(v1.Data(), v1.Data() + v1.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 1 + 2*2 + 4*4);

        cb.Clear();
        v1.DotProductWithCallback<true>(v1.Data(), v1.Data() + v1.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 1 + 2*2 + 4*4);

        cb.Clear();
        empty.DotProductWithCallback<false>(v1.Data(), v1.Data() + v1.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 3);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 0);

        cb.Clear();
        empty.DotProductWithCallback<true>(v1.Data(), v1.Data() + v1.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 0);

        cb.Clear();
        v1.DotProductWithCallback<false>(empty.Data(), empty.Data() + empty.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 3);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 0);

        cb.Clear();
        v1.DotProductWithCallback<true>(empty.Data(), empty.Data() + empty.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 3);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 0);

        word2weight1[3] = 3;
        word2weight1[5] = 5;
        TBinarizedWordHashVector v2(word2weight1);

        cb.Clear();
        v2.DotProductWithCallback<false>(v1.Data(), v1.Data() + v1.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 2);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 1 + 2*2 + 4*4);

        cb.Clear();
        v2.DotProductWithCallback<true>(v1.Data(), v1.Data() + v1.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 2);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 1 + 2*2 + 4*4);

        cb.Clear();
        v1.DotProductWithCallback<false>(v2.Data(), v2.Data() + v2.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 2);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 1 + 2*2 + 4*4);

        cb.Clear();
        v1.DotProductWithCallback<true>(v2.Data(), v2.Data() + v2.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 1 + 2*2 + 4*4);

        word2weight1[6] = 6;
        word2weight1[3] = 2;
        word2weight1[4] = 3;
        word2weight1.erase(1);
        TBinarizedWordHashVector v3(word2weight1);

        cb.Clear();
        v2.DotProductWithCallback<false>(v3.Data(), v3.Data() + v3.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 1);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 1);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 2*2 + 2*3  + 3*4 + 5*5);

        cb.Clear();
        v2.DotProductWithCallback<true>(v3.Data(), v3.Data() + v3.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 1);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 2*2 + 2*3  + 3*4 + 5*5);

        cb.Clear();
        v3.DotProductWithCallback<false>(v2.Data(), v2.Data() + v2.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 1);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 1);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 2*2 + 2*3  + 3*4 + 5*5);

        cb.Clear();
        v3.DotProductWithCallback<true>(v2.Data(), v2.Data() + v2.Size(), cb);
        UNIT_ASSERT_EQUAL(cb.LeftZeroes, 0);
        UNIT_ASSERT_EQUAL(cb.RightZeroes, 1);
        UNIT_ASSERT_EQUAL(cb.MatchedAccumulator, 2*2 + 2*3  + 3*4 + 5*5);
    }

    Y_UNIT_TEST(TestDotProduct) {
        TWordVector v1(StringSplitter("a b c d d d e f").Split(' ').ToList<TString>());
        TWordVector v2(StringSplitter("a b c d d e f g h i j k l m n o p").Split(' ').ToList<TString>());

        TBinarizedWordHashVector binV1(v1.GetWeights());
        TBinarizedWordHashVector binV2(v2.GetWeights());

        const double dotProduct = v1 * v2;
        const double dotProduct2 = TBinarizedWordHashVector::Normalize(binV1 * binV2);
        UNIT_ASSERT_DOUBLES_EQUAL(dotProduct, dotProduct2, 0.002);

        const auto callback = [](const THashWithWeight*, const THashWithWeight*) -> void {};
        const double dotProductRawPointers = TBinarizedWordHashVector::Normalize(DotProductWithCallback<false>(
                binV1.Data(), binV1.Data() + binV1.Size(),
                binV2.Data(), binV2.Data() + binV2.Size(),
                callback));

        UNIT_ASSERT_EQUAL(dotProduct2, dotProductRawPointers);
    }

    namespace {
        ui32 HashString(TStringBuf s) {
            return NVowpalWabbit::HashString(s.begin(), s.end(), 0);
        }
    }

    Y_UNIT_TEST(TestHashCollisions) {
        TWordVector v1({"а", "а", "б", "б", "в", "в"});
        TWordVector v2({"а", ToString(HashString("а")),
                        "б", ToString(HashString("б")),
                        "в", ToString(HashString("в"))});

        TBinarizedWordHashVector bv1(v1.GetWeights());
        TBinarizedWordHashVector bv2(v2.GetWeights());

        UNIT_ASSERT_EQUAL(v1.Size(), 3);
        UNIT_ASSERT_EQUAL(v2.Size(), 6);

        UNIT_ASSERT_EQUAL(bv1.Size(), 3);
        UNIT_ASSERT_EQUAL(bv2.Size(), 3);

        UNIT_ASSERT(bv1 * bv1 < bv2 * bv2);
    }

    Y_UNIT_TEST(TestHashCollisions2) {
        TWordVector v({"а", ToString(HashString("а"))});
        TBinarizedWordHashVector bv(v.GetWeights());

        UNIT_ASSERT_EQUAL(v.Size(), 2);
        UNIT_ASSERT_EQUAL(bv.Size(), 1);
        UNIT_ASSERT_EQUAL(bv.Data()->Weight, TBinarizedWordHashVector::MaxWeight);
    }
}
