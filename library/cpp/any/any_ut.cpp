#include "any.h"

#include <util/generic/hash.h>
#include <util/generic/singleton.h>

#include <library/cpp/testing/unittest/registar.h>

class TAnyTest: public TTestBase {
    UNIT_TEST_SUITE(TAnyTest);
    UNIT_TEST(TestAssign);
    UNIT_TEST(TestMethods);
    UNIT_TEST_SUITE_END();

public:
    struct TVPHash {
        size_t operator()(const void* p) const {
            return NumericHash(reinterpret_cast<i64>(p));
        }
    };

    using TMemoryCnt = THashMap<const void*, i64, TVPHash>;

    struct TMemoryTest {
        i64 Num;

        TMemoryTest()
            : Num()
        {
            static i64 cnt = 0;
            Num = cnt++;
            UNIT_ASSERT_VALUES_EQUAL(0, GetCnt());
            GetCnt() += 1;
        }

        TMemoryTest(const TMemoryTest& t)
            : Num(t.Num)
        {
            UNIT_ASSERT_VALUES_EQUAL(0, GetCnt());
            GetCnt() += 1;
        }

        ~TMemoryTest() {
            UNIT_ASSERT_VALUES_EQUAL(1, GetCnt());
            GetCnt() -= 1;
        }

        bool operator==(const TMemoryTest& t) const {
            return t.Num == Num;
        }

        i64& GetCnt() const {
            return (*Singleton<TMemoryCnt>())[(const void*)this];
        }
    };

    void VerifyMemory() {
        const TMemoryCnt& cnt = *Singleton<TMemoryCnt>();
        for (TMemoryCnt::const_iterator it = cnt.begin(); it != cnt.end(); ++it) {
            UNIT_ASSERT_VALUES_EQUAL_C(0, it->second, it->first);
        }
    }

    template <typename T>
    void DoTestAny(NAny::TAny& any, const T& val) {
        any = val;
        UNIT_ASSERT_VALUES_EQUAL(val, any.Cast<T>());
        UNIT_ASSERT_VALUES_EQUAL(val, ((const NAny::TAny&)any).Cast<T>());
        UNIT_ASSERT(any.Compatible(val));
        UNIT_ASSERT(any.Compatible<T>());
        UNIT_ASSERT(any.Compatible(any));
        UNIT_ASSERT(!any.Compatible(NAny::TAny()));
        UNIT_ASSERT(!any.Empty());
    }

    void TestAssign() {
        {
            NAny::TAny any;
            UNIT_ASSERT(any.Empty());
            UNIT_ASSERT(any.Compatible(NAny::TAny()));
        }

        {
            NAny::TAny any = TMemoryTest();
            TMemoryTest test;
            TSimpleSharedPtr<TMemoryTest> test1 = new TMemoryTest;
            TSimpleSharedPtr<TMemoryTest> test2;
            DoTestAny(any, true);
            DoTestAny(any, test);
            DoTestAny(any, TString("test"));
            DoTestAny(any, 'a');
            DoTestAny(any, (ui8)'c');
            DoTestAny(any, (i8)'b');
            DoTestAny(any, (i16)11);
            DoTestAny(any, (ui16)12);
            DoTestAny(any, (i32)-1);
            DoTestAny(any, (ui32)-123443);
            any = NAny::TAny();
            DoTestAny(any, test);
            DoTestAny(any, TMemoryTest());
            DoTestAny(any, (i64)-3535243);
            DoTestAny(any, (ui64)-645289032);
            DoTestAny(any, test1);
            DoTestAny(any, test2);
            DoTestAny(any, &test1);
        }

        VerifyMemory();
    }

    void TestMethods() {
        NAny::TAny a('a'), b;
        b.Swap(a);
        b.Cast<char>() = 'c';
        UNIT_ASSERT_VALUES_EQUAL('c', b.Cast<char>());

        a = TString("aa");
        b.Swap(a);
        b.Cast<TString>() = "cc";
        UNIT_ASSERT_VALUES_EQUAL("cc", b.Cast<TString>());
    }
};

template <>
void Out<TAnyTest::TMemoryTest>(IOutputStream& out, TTypeTraits<TAnyTest::TMemoryTest>::TFuncParam t) {
    out << "TMemoryTest." << t.Num;
}

template <>
void Out<TSimpleSharedPtr<TAnyTest::TMemoryTest>>(IOutputStream& out, TTypeTraits<TSimpleSharedPtr<TAnyTest::TMemoryTest>>::TFuncParam t) {
    out << "&TMemoryTest->";
    if (!t)
        out << "null";
    else
        out << t->Num;
}

template <>
void Out<TSimpleSharedPtr<TAnyTest::TMemoryTest>*>(IOutputStream& out, TTypeTraits<TSimpleSharedPtr<TAnyTest::TMemoryTest>*>::TFuncParam t) {
    out << "&&TMemoryTest->";
    if (!t)
        out << "null";
    else if (!(*t))
        out << "->null";
    else
        out << "->" << (*t)->Num;
}

UNIT_TEST_SUITE_REGISTRATION(TAnyTest)
