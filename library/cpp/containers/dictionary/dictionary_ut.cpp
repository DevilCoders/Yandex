#include "dictionary.h"

#include <library/cpp/testing/unittest/registar.h>

enum ESample {
    ES_0,
    ES_1,
    ES_2,
    ES_3,
    ES_4,

    ES_MAX
};

const std::pair<const char*, ESample> ArrayInOrder[] = {
    std::make_pair("es_0", ES_0),
    std::make_pair("eS_1", ES_1),
    std::make_pair("Es_2", ES_2),
};

const std::pair<const char*, ESample> ArrayOutOfOrder[] = {
    std::make_pair("es_0", ES_0),
    std::make_pair("Es_2", ES_2),
    std::make_pair("eS_1", ES_1),
};

const std::pair<const char*, ESample> ArrayWithGap[] = {
    std::make_pair("es_0", ES_0),
    std::make_pair("eS_1", ES_1),
    std::make_pair("es_3", ES_3),
};

const std::pair<const char*, ESample> ArrayWithDoubleString[] = {
    std::make_pair("es_0", ES_0),
    std::make_pair("eS_1", ES_1),
    std::make_pair("Es_2", ES_2),
    std::make_pair("eS_1", ES_3),
};

const std::pair<const char*, ESample> ArrayWithDoubleVal[] = {
    std::make_pair("es_0", ES_0),
    std::make_pair("eS_1", ES_1),
    std::make_pair("Es_2", ES_2),
    std::make_pair("es_3", ES_1),
};

template <class TDict>
void CheckPair(const TDict& dict, const std::pair<const char*, ESample>& pair, bool caseSens) {
    UNIT_ASSERT_EQUAL_C(dict.Name2Id(pair.first), pair.second, pair.first);
    UNIT_ASSERT_EQUAL_C(dict.Id2Name(pair.second), pair.first, pair.first);
    if (caseSens) {
        UNIT_ASSERT_EXCEPTION(dict.Name2Id(to_upper(TString(pair.first))), yexception);
    } else {
        UNIT_ASSERT_EQUAL_C(dict.Name2Id(to_upper(TString(pair.first))), pair.second, pair.first);
    }
}

template <class TDict>
void CheckDictSub_(const TDict& dict, const std::pair<const char*, ESample> array[], size_t arraySize, bool caseSens) {
    const size_t size = dict.Size();

    for (size_t i = 0; i < arraySize; ++i) {
        CheckPair(dict, array[i], caseSens);
    }

    UNIT_ASSERT_EQUAL(dict.Size(), size);
}

template <class TDict>
void CheckDict_(const TDict& dict, const std::pair<const char*, ESample> array[], size_t arraySize, bool caseSens) {
    UNIT_ASSERT_EQUAL(dict.Size(), arraySize);
    CheckDictSub_(dict, array, arraySize, caseSens);
}

#define CheckDict(dict, Array, caseSens) \
    CheckDict_(dict, Array, Y_ARRAY_SIZE(Array), caseSens)

#define CheckDictSub(dict, Array, caseSens) \
    CheckDictSub_(dict, Array, Y_ARRAY_SIZE(Array), caseSens)

#define NewDict(TDict, Array) \
    TAutoPtr<TDict>(new TDict(Array))

#define InitDict(dict, Array) \
    dict.Init(Array)

#define InsertInitDict(dict, Array)                       \
    {                                                     \
        for (size_t i = 0; i < Y_ARRAY_SIZE(Array); ++i)  \
            dict.Insert(Array[i].first, Array[i].second); \
    }

#define UNIT_ASSERT_CREATION(TDict, Array, caseSens) \
    {                                                \
        TAutoPtr<TDict> ptr = NewDict(TDict, Array); \
        UNIT_ASSERT(ptr);                            \
        CheckDict(*ptr, Array, caseSens);            \
    }

#define UNIT_ASSERT_INIT(TDict, Array, caseSens) \
    {                                            \
        TDict dict;                              \
        InitDict(dict, Array);                   \
        CheckDict(dict, Array, caseSens);        \
    }

#define UNIT_ASSERT_INSERT_INIT(TDict, Array, caseSens) \
    {                                                   \
        TDict dict;                                     \
        InsertInitDict(dict, Array);                    \
        CheckDict(dict, Array, caseSens);               \
    }

#define UNIT_ASSERT_INIT_FAIL(TDict, Array)                       \
    {                                                             \
        TDict dict;                                               \
        UNIT_ASSERT_EXCEPTION(InitDict(dict, Array), yexception); \
    }

#define UNIT_ASSERT_INSERT_INIT_FAIL(TDict, Array)                      \
    {                                                                   \
        TDict dict;                                                     \
        UNIT_ASSERT_EXCEPTION(InsertInitDict(dict, Array), yexception); \
    }

template <typename StrT>
class TStringIdDictionaryTestBase: public TTestBase {
public:
    using TStringIdDict = TEnum2String<ESample, StrT, NNameIdDictionary::TSolidId2Str<ESample>>;
    using TStringSparseIdDict = TEnum2String<ESample, StrT, NNameIdDictionary::TSparseId2Str<ESample>>;
    using TCIStringIdDict = TEnum2String<ESample, StrT, NNameIdDictionary::TSolidId2Str<ESample>, NNameIdDictionary::TCaseInsensitiveStr2Id<ESample>>;
    using TCIStringSparseIdDict = TEnum2String<ESample, StrT, NNameIdDictionary::TSparseId2Str<ESample>, NNameIdDictionary::TCaseInsensitiveStr2Id<ESample>>;

    void TestInit() {
        TTestInit<TStringIdDict, true>();
        TTestInit<TCIStringIdDict, false>();
        TTestInitSparse<TStringSparseIdDict, true>();
        TTestInitSparse<TCIStringSparseIdDict, false>();
    }

    void TestInsert() {
        TTestInsertSolid<TStringIdDict, true>();
        TTestInsertSolid<TCIStringIdDict, false>();
        TTestInsertSparse<TStringSparseIdDict, true>();
        TTestInsertSparse<TCIStringSparseIdDict, false>();
    }

    template <class TDict, bool CaseSens>
    void TTestInit() {
        UNIT_ASSERT_CREATION(TDict, ArrayInOrder, CaseSens);
        UNIT_ASSERT_EXCEPTION(NewDict(TDict, ArrayOutOfOrder), yexception);
        UNIT_ASSERT_EXCEPTION(NewDict(TDict, ArrayWithGap), yexception);
        UNIT_ASSERT_EXCEPTION(NewDict(TDict, ArrayWithDoubleString), yexception);
        UNIT_ASSERT_EXCEPTION(NewDict(TDict, ArrayWithDoubleVal), yexception);

        UNIT_ASSERT_INIT(TDict, ArrayInOrder, CaseSens);
        UNIT_ASSERT_INIT_FAIL(TDict, ArrayOutOfOrder);
        UNIT_ASSERT_INIT_FAIL(TDict, ArrayWithGap);
        UNIT_ASSERT_INIT_FAIL(TDict, ArrayWithDoubleString);
        UNIT_ASSERT_INIT_FAIL(TDict, ArrayWithDoubleVal);

        UNIT_ASSERT_INSERT_INIT(TDict, ArrayInOrder, CaseSens);
#ifndef _win_ // doesn't compile on Windows
#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wcompound-token-split-by-macro"
#endif
        UNIT_ASSERT_INSERT_INIT_FAIL(TDict, ArrayOutOfOrder);
        UNIT_ASSERT_INSERT_INIT_FAIL(TDict, ArrayWithGap);
        UNIT_ASSERT_INSERT_INIT_FAIL(TDict, ArrayWithDoubleString);
        UNIT_ASSERT_INSERT_INIT_FAIL(TDict, ArrayWithDoubleVal);
#ifdef __clang__
# pragma clang diagnostic pop
#endif
#endif
    }

    template <class TDict, bool CaseSens>
    void TTestInitSparse() {
        UNIT_ASSERT_CREATION(TDict, ArrayInOrder, CaseSens);
        UNIT_ASSERT_CREATION(TDict, ArrayOutOfOrder, CaseSens);
        UNIT_ASSERT_CREATION(TDict, ArrayWithGap, CaseSens);
        UNIT_ASSERT_EXCEPTION(NewDict(TDict, ArrayWithDoubleString), yexception);
        UNIT_ASSERT_EXCEPTION(NewDict(TDict, ArrayWithDoubleVal), yexception);

        UNIT_ASSERT_INIT(TDict, ArrayInOrder, CaseSens);
        UNIT_ASSERT_INIT(TDict, ArrayOutOfOrder, CaseSens);
        UNIT_ASSERT_INIT(TDict, ArrayWithGap, CaseSens);
        UNIT_ASSERT_INIT_FAIL(TDict, ArrayWithDoubleString);
        UNIT_ASSERT_INIT_FAIL(TDict, ArrayWithDoubleVal);

        UNIT_ASSERT_INSERT_INIT(TDict, ArrayInOrder, CaseSens);
        UNIT_ASSERT_INSERT_INIT(TDict, ArrayOutOfOrder, CaseSens);
        UNIT_ASSERT_INSERT_INIT(TDict, ArrayWithGap, CaseSens);
#ifndef _win_ // doesn't compile on Windows
#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wcompound-token-split-by-macro"
#endif
        UNIT_ASSERT_INSERT_INIT_FAIL(TDict, ArrayWithDoubleString);
        UNIT_ASSERT_INSERT_INIT_FAIL(TDict, ArrayWithDoubleVal);
#ifdef __clang__
# pragma clang diagnostic pop
#endif
#endif
    }

    template <class TDict>
    void TestInsert(TDict& dict, const std::pair<const char*, ESample> array[], size_t arraySize, bool caseSens) {
        CheckDict_(dict, array, arraySize, caseSens);
        UNIT_ASSERT_EXCEPTION(dict.Insert("es_3", ES_2), yexception);
        UNIT_ASSERT_EXCEPTION(dict.Insert("Es_2", ES_3), yexception);
        CheckDict_(dict, array, arraySize, caseSens);
        dict.Insert("es_3", ES_3);
        CheckPair(dict, std::make_pair("es_3", ES_3), caseSens);
        UNIT_ASSERT_EQUAL(dict.Size(), arraySize + 1);
        CheckDictSub_(dict, array, arraySize, caseSens);
    }

    template <class TDict, bool CaseSens>
    void TTestInsertSolid() {
        TAutoPtr<TDict> ptr = NewDict(TDict, ArrayInOrder);
        CheckDict(*ptr, ArrayInOrder, CaseSens);
        UNIT_ASSERT_EXCEPTION(ptr->Insert("es_4", ES_4), yexception);
        TestInsert(*ptr, ArrayInOrder, Y_ARRAY_SIZE(ArrayInOrder), CaseSens);
    }

    template <class TDict, bool CaseSens>
    void TTestInsertSparse() {
        TAutoPtr<TDict> ptr = NewDict(TDict, ArrayInOrder);
        TestInsert(*ptr, ArrayInOrder, Y_ARRAY_SIZE(ArrayInOrder), CaseSens);

        TAutoPtr<TDict> ptr2 = NewDict(TDict, ArrayWithGap);
        CheckDict(*ptr2, ArrayWithGap, CaseSens);
        ptr2->Insert("Es_2", ES_2);
        CheckPair(*ptr2, std::make_pair("Es_2", ES_2), CaseSens);
        UNIT_ASSERT_EQUAL(ptr2->Size(), Y_ARRAY_SIZE(ArrayWithGap) + 1);
        CheckDictSub(*ptr2, ArrayWithGap, CaseSens);
    }
};

class TStringIdDictionaryTest_TStrBuf: public TStringIdDictionaryTestBase<TStringBuf> {
    UNIT_TEST_SUITE(TStringIdDictionaryTest_TStrBuf);
    UNIT_TEST(TestInit);
    UNIT_TEST(TestInsert);
    UNIT_TEST_SUITE_END();
};

class TStringIdDictionaryTest_TString: public TStringIdDictionaryTestBase<TStringBuf> {
    UNIT_TEST_SUITE(TStringIdDictionaryTest_TString);
    UNIT_TEST(TestInit);
    UNIT_TEST(TestInsert);
    UNIT_TEST_SUITE_END();
};

UNIT_TEST_SUITE_REGISTRATION(TStringIdDictionaryTest_TStrBuf)
UNIT_TEST_SUITE_REGISTRATION(TStringIdDictionaryTest_TString)
