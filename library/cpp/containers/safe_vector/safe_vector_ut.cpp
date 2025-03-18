#include <safe_vector.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>

class TSafeVectorTest: public TTestBase {
private:
    UNIT_TEST_SUITE(TSafeVectorTest);
    UNIT_TEST(TestConstructor);
    UNIT_TEST(TestOperatorSquareBrackets);
    UNIT_TEST(TestFront);
    UNIT_TEST(TestBack);
    UNIT_TEST(TestAt);
    UNIT_TEST(TestInsert);
    UNIT_TEST(TestErase);
    UNIT_TEST(TestPopBack);
    UNIT_TEST(TestAsVector);
    UNIT_TEST(TestSerialize);
    UNIT_TEST_SUITE_END();

public:
    void TestConstructor() {
        TSafeVector<int> v(10);
        UNIT_ASSERT_VALUES_EQUAL(v.size(), 10);
        UNIT_ASSERT_VALUES_EQUAL(v[1], 0);
        UNIT_ASSERT_VALUES_EQUAL(v[9], 0);

        TSafeVector<int> v1(v.begin(), v.end());
        UNIT_ASSERT_VALUES_EQUAL(v1.size(), 10);
        UNIT_ASSERT_VALUES_EQUAL(v1[1], 0);
        UNIT_ASSERT_VALUES_EQUAL(v1[9], 0);
        UNIT_ASSERT_VALUES_EQUAL(v1[5], v[5]);

        TSafeVector<int> v2(v);
        UNIT_ASSERT_VALUES_EQUAL(v2.size(), 10);
        UNIT_ASSERT_VALUES_EQUAL(v2[1], 0);
        UNIT_ASSERT_VALUES_EQUAL(v2[9], 0);
        UNIT_ASSERT_VALUES_EQUAL(v2[3], v1[3]);

        TSafeVector<int> v3;
        v3 = v2;
        UNIT_ASSERT_VALUES_EQUAL(v3.size(), 10);
        UNIT_ASSERT_VALUES_EQUAL(v3[1], 0);
        UNIT_ASSERT_VALUES_EQUAL(v3[9], 0);
        UNIT_ASSERT_VALUES_EQUAL(v3[3], v1[3]);
    }

    void TestOperatorSquareBrackets() {
        TSafeVector<int> v;
        v.push_back(1);
        v.push_back(2);

        UNIT_ASSERT_VALUES_EQUAL(v.size(), 2);

        UNIT_CHECK_GENERATED_EXCEPTION(v[3] = 100, yexception);
    }

    void TestFront() {
        TSafeVector<int> v;
        UNIT_CHECK_GENERATED_EXCEPTION(v.front() = 234, yexception);
    }

    void TestBack() {
        TSafeVector<int> v;
        UNIT_CHECK_GENERATED_EXCEPTION(v.back() = 234, yexception);
    }

    void TestAt() {
        TSafeVector<int> v(5, 10);
        for (size_t idx = 0; idx < v.size(); ++idx)
            UNIT_CHECK_GENERATED_NO_EXCEPTION(v.at(idx), yexception);

        UNIT_ASSERT_VALUES_EQUAL(v.at(3), 10);

        UNIT_CHECK_GENERATED_EXCEPTION(v.at(6), yexception);
        UNIT_CHECK_GENERATED_EXCEPTION(v.at(v.size() + 5), yexception);
    }

    void TestInsert() {
        TSafeVector<int> v;
        v.insert(v.begin(), 12);
        v.insert(v.end(), 14);
        UNIT_CHECK_GENERATED_NO_EXCEPTION(v.insert(v.begin() + 1, 13), yexception);

        UNIT_ASSERT_VALUES_EQUAL(v.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(v[1], 13);

        UNIT_CHECK_GENERATED_EXCEPTION(v.insert(v.begin() + 4, 15), yexception);
    }

    void TestErase() {
        TSafeVector<int> v;
        v.insert(v.begin(), 10, 5);
        UNIT_ASSERT_VALUES_EQUAL(v.size(), 10);

        UNIT_CHECK_GENERATED_NO_EXCEPTION(v.erase(v.begin() + 4, v.end()), yexception);

        UNIT_ASSERT_VALUES_EQUAL(v.size(), 4);

        UNIT_CHECK_GENERATED_EXCEPTION(v.erase(v.begin() + 3, v.begin() + 6), yexception);
    }

    void TestPopBack() {
        TSafeVector<int> v(2, 10);
        UNIT_ASSERT_VALUES_EQUAL(v[1], 10);
        int currSize = 2;
        UNIT_ASSERT_VALUES_EQUAL(v.size(), currSize);

        while (currSize) {
            UNIT_CHECK_GENERATED_NO_EXCEPTION(v.pop_back(), yexception);
            --currSize;
            UNIT_ASSERT_VALUES_EQUAL(v.size(), currSize);
        }

        UNIT_CHECK_GENERATED_EXCEPTION(v.pop_back(), yexception);
    }

    void TestAsVector() {
        TSafeVector<int> v(2, 10);
        TVector<int>& yv = v.AsVector();
        UNIT_ASSERT_VALUES_EQUAL(yv.size(), v.size());
        UNIT_ASSERT_VALUES_EQUAL(yv[1], 10);
        yv[0] = 125;
        UNIT_ASSERT_VALUES_EQUAL(v[0], 125);

        const TVector<int>& yvConst = v.AsVector();
        UNIT_ASSERT_VALUES_EQUAL(yvConst.size(), v.size());
        UNIT_ASSERT_VALUES_EQUAL(yvConst[0], 125);
        UNIT_ASSERT_VALUES_EQUAL(yvConst[1], 10);

        TSafeVector<TAutoPtr<int>> vAuto;
        vAuto.push_back(TAutoPtr<int>(new int(3)));
        TVector<TAutoPtr<int>>& yvAuto = vAuto.AsVector();
        UNIT_ASSERT_VALUES_EQUAL(yvAuto.size(), vAuto.size());
        UNIT_ASSERT_VALUES_EQUAL(*yvAuto[0], 3);
        *yvAuto[0] = 123;
        UNIT_ASSERT_VALUES_EQUAL(*vAuto[0], 123);
    }

    void TestSerialize() {
        TSafeVector<int> v(2, 10);
        TSafeVector<int> q;
        v[1] = 123;

        TStringStream stream;
        ::Save(&stream, v);
        ::Load(&stream, q);

        UNIT_ASSERT_VALUES_EQUAL(v.size(), q.size());
        UNIT_ASSERT_VALUES_EQUAL(v[1], q[1]);
        UNIT_ASSERT_VALUES_EQUAL(q[1], 123);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TSafeVectorTest);
