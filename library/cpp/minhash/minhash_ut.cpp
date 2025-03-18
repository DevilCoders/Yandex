#include "minhash_builder.h"
#include "minhash_func.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/map.h>
#include <util/stream/buffer.h>
#include <util/generic/algorithm.h>
#include <util/random/random.h>

static const char* episodes[] = {
    "Enter: Naruto Uzumaki!",
    "My Name is Konohamaru!",
    "Sasuke and Sakura: Friends or Foes?",
    "Pass or Fail: Survival Test",
    "You Failed! Kakashi's Final Decision",
    "A Dangerous Mission! Journey to the Land of Waves!",
    "The Assassin of the Mist!",
    "The Oath of Pain",
    "Kakashi: Sharingan Warrior!",
    "The Forest of Chakra",
    "The Land Where a Hero Once Lived",
    "Battle on the Bridge! Zabuza Returns!",
    "Haku's Secret Jutsu: Crystal Ice Mirrors",
    "The Number One Hyperactive, Knucklehead Ninja Joins the Fight!",
    "Zero Visibility: The Sharingan Shatters",
    "The Broken Seal",
    "White Past: Hidden Ambition",
    "The Weapons Known as Shinobi",
    "The Demon in the Snow",
    "A New Chapter Begins: The Chunin Exam!",
    "Identify Yourself: Powerful New Rivals",
    "Chunin Challenge: Rock Lee vs. Sasuke!",
    "Genin Takedown! All Nine Rookies Face Off!",
    "Start Your Engines: The Chunin Exam Begins!",
    "The Tenth Question: All or Nothing!",
    "Special Report: Live from the Forest of Death!"};

class TMinHashTest: public TTestBase {
    UNIT_TEST_SUITE(TMinHashTest);
    UNIT_TEST(TestHash1);
    UNIT_TEST(TestHash2);
    UNIT_TEST(TestHash3);
    UNIT_TEST(TestHash4);
    UNIT_TEST(TestHash5);
    UNIT_TEST(TestHash6);
    UNIT_TEST(TestHash7);
    UNIT_TEST(TestHash8);
    UNIT_TEST(TestHash9);
    UNIT_TEST(TestHash10);
    UNIT_TEST(TestHash11);
    UNIT_TEST(TestHash12);
    UNIT_TEST(TestHash13);
    UNIT_TEST_SUITE_END();

public:
    void SetUp() override;
    TVector<TString> Episodes;

    void TestHash(const TVector<TString>& data,
                  const TVector<ui32> expected,
                  double loadFactor, ui32 keysPerBin, ui32 seed);
    void TestHash(const TVector<TString>& data,
                  double loadFactor, ui32 keysPerBin, ui32 seed, ui8 fprSize = 0, const TVector<TString>& negativeKeys = TVector<TString>());

    void TestHash1();
    void TestHash2();
    void TestHash3();
    void TestHash4();
    void TestHash5();
    void TestHash6();
    void TestHash7();
    void TestHash8();
    void TestHash9();
    void TestHash10();
    void TestHash11();
    void TestHash12();
    void TestHash13();
};

UNIT_TEST_SUITE_REGISTRATION(TMinHashTest);

using namespace NMinHash;

#define VECTOR_DATA(name, len, prefix)              \
    TVector<TString> name;                          \
    for (ui32 i = 0; i < len; ++i) {                \
        name.push_back(prefix + ToString<ui32>(i)); \
    }

#define RANDOM_VECTOR_DATA(name, len)                       \
    TVector<TString> name;                                  \
    {                                                       \
        for (int i = 0; i < len; ++i)                       \
            name.push_back(NUnitTest::RandomString(23, i)); \
    }

#define ARRAY_DATA(type, name, ...)             \
    static const type name##_a[] = __VA_ARGS__; \
    TVector<type> name(name##_a, name##_a + sizeof(name##_a) / sizeof(*name##_a))

void TMinHashTest::SetUp() {
    Episodes = TVector<TString>(episodes, episodes + Y_ARRAY_SIZE(episodes));
}

void TMinHashTest::TestHash(const TVector<TString>& data,
                            const TVector<ui32> expected,
                            double loadFactor, ui32 keysPerBin, ui32 seed) {
    TChdHashBuilder builder(data.size(), loadFactor, keysPerBin, seed);
    TBufferStream bf;
    builder.Build(data, &bf);
    TChdMinHashFunc hash(&bf);

    for (size_t i = 0; i < expected.size(); ++i) {
        ui32 pos = hash.Get(data[i].data(), data[i].size());
        UNIT_ASSERT_EQUAL(pos, expected[i]);
    }
}

void TMinHashTest::TestHash(const TVector<TString>& data,
                            double loadFactor, ui32 keysPerBin, ui32 seed, ui8 fprSize, const TVector<TString>& negativeKeys) {
    TChdHashBuilder builder(data.size(), loadFactor, keysPerBin, seed, fprSize);
    TBufferStream bf;
    builder.Build(data, &bf);
    TChdMinHashFunc hash(&bf);

    TVector<ui32> result;
    for (size_t i = 0; i < data.size(); ++i)
        result.push_back(hash.Get(data[i].data(), data[i].size()));
    Sort(result.begin(), result.end());
    for (size_t i = 0; i < result.size(); ++i) {
        UNIT_ASSERT_EQUAL(i, result[i]);
    }
    for (size_t i = 0; i < negativeKeys.size(); ++i) {
        UNIT_ASSERT_EQUAL(hash.Get(negativeKeys[i].data(), negativeKeys[i].size()), TChdMinHashFunc::npos);
    }
}

void TMinHashTest::TestHash1() {
    ARRAY_DATA(ui32, expected, {2, 3, 18, 6, 9, 26, 22, 12, 10, 7, 28, 13, 0, 23, 4, 24, 11, 15, 27, 21, 19, 17, 8, 5, 25, 1});
    TestHash(Episodes, /* expected,*/ 0.99, 5, 13);
}

void TMinHashTest::TestHash2() {
    VECTOR_DATA(data, 1000, "some key ");
    TestHash(data, 0.8, 5, 2);
}

void TMinHashTest::TestHash3() {
    VECTOR_DATA(data, 10000, "some key ");
    TestHash(data, 0.8, 5, 2);
}

void TMinHashTest::TestHash4() {
    VECTOR_DATA(data, 100000, "some key ");
    TestHash(data, 0.8, 5, 2);
}

void TMinHashTest::TestHash5() {
    RANDOM_VECTOR_DATA(data, 1000);
    TestHash(data, 0.8, 5, 2);
}

void TMinHashTest::TestHash6() {
    RANDOM_VECTOR_DATA(data, 10000);
    TestHash(data, 0.8, 5, 2);
}

void TMinHashTest::TestHash7() {
    RANDOM_VECTOR_DATA(data, +0);
    TestHash(data, 0.0, 1, 2);
}

void TMinHashTest::TestHash8() {
    VECTOR_DATA(data, 1000, "some key ");
    VECTOR_DATA(negative, 1000, "another key ");
    TestHash(data, 0.8, 5, 2, 32, negative);
}

void TMinHashTest::TestHash9() {
    TVector<TString> keys[2];
    for (size_t i = 0; i < Episodes.size(); ++i)
        keys[i % 2].push_back(Episodes[i]);
    TestHash(keys[0], 0.8, 5, 2, 32, keys[1]);
    TestHash(keys[1], 0.8, 5, 2, 32, keys[0]);
}

void TMinHashTest::TestHash10() {
    RANDOM_VECTOR_DATA(data, 1);
    TestHash(data, 0.9, 1, 2);
}

void TMinHashTest::TestHash11() {
    RANDOM_VECTOR_DATA(data, 2);
    TestHash(data, 0.9, 1, 2);
}

void TMinHashTest::TestHash12() {
    RANDOM_VECTOR_DATA(data, 1);
    TestHash(data, 0.9, 5, 1);
}

void TMinHashTest::TestHash13() {
    TVector<TString> keys = {"DA37A51D13BBE59842E2499526FB16D7", "A3B786CECCFB58EDE4CB7D2830C59C31"};
    TestHash(keys, 0.99999, 5, 1);
}
#undef VECTOR_DATA
#undef RANDOM_VECTOR_DATA
#undef ARRAY_DATA
