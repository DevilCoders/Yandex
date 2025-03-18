#include <library/cpp/testing/unittest/registar.h>

#include "nth_elements.h"


Y_UNIT_TEST_SUITE(TNthElements) {
    Y_UNIT_TEST(Generic) {
        std::srand(42);
        {
            TVector<TString> v;
            TVector<TVector<TString>::iterator> poses;
            NthElements(v.begin(), v.end(), poses.begin(), poses.end());
            UNIT_ASSERT_EQUAL(v, TVector<TString>());
        }

        {
            TVector<TString> v;
            TVector<TVector<TString>::iterator> poses = {v.end()};
            NthElements(v.begin(), v.end(), poses.begin(), poses.end());
            UNIT_ASSERT_EQUAL(v, TVector<TString>());
        }

        {
            TVector<int> v = {1};
            TVector<TVector<int>::iterator> poses = {v.begin()};
            NthElements(v.begin(), v.end(), poses.begin(), poses.end());
            UNIT_ASSERT_VALUES_EQUAL(v[0], 1);
            NthElements(v.begin(), v.end(), poses.begin(), poses.end(),
                        TGreater<>());
            UNIT_ASSERT_VALUES_EQUAL(v[0], 1);
        }

        {
            TVector<int> v = {1, 2};
            TVector<TVector<int>::iterator> poses = {v.begin()};
            NthElements(v.begin(), v.end(), poses.begin(), poses.end());
            UNIT_ASSERT_VALUES_EQUAL(v[0], 1);
            UNIT_ASSERT_VALUES_EQUAL(v[1], 2);
            NthElements(v.begin(), v.end(), poses.begin(), poses.end(), TGreater<>());
            UNIT_ASSERT_VALUES_EQUAL(v[0], 2);
            UNIT_ASSERT_VALUES_EQUAL(v[1], 1);
        }

        {
            std::srand(42);
            int data[] = {3, 2, 1, 4, 6, 5, 7, 9, 8};
            TVector<int> testVector(data, data + Y_ARRAY_SIZE(data));

            size_t medianInd = testVector.size() / 2;

            TVector<TVector<int>::iterator> poses = {testVector.begin() + medianInd};
            NthElements(testVector.begin(), testVector.end(),
                        poses.begin(), poses.end());
            UNIT_ASSERT_VALUES_EQUAL(testVector[medianInd], 5);

            NthElements(testVector.begin(), testVector.end(),
                        poses.begin(), poses.end(),
                        [](int lhs, int rhs) { return lhs > rhs; });
            UNIT_ASSERT_VALUES_EQUAL(testVector[medianInd], 5);
        }

        {
            std::srand(960);
            TVector<size_t> v = {742, 776, 808, 360, 184, 360, 0, 712, 590, 0};
            TVector<size_t> indexes = {6};

            TVector<TVector<size_t>::iterator> poses;
            Transform(indexes.begin(), indexes.end(), std::back_inserter(poses),
                      [&v](auto index) { return v.begin() + index; });

            NthElements(v.begin(), v.end(),
                        poses.begin(), poses.end(),
                        TLess<>());
            UNIT_ASSERT_VALUES_EQUAL(v[6], 712);
        }

        for (int i = 0; i < 10; ++i) {
            TVector<int> v(1e2);
            Generate(v.begin(), v.end(), std::rand);
            TVector<TVector<int>::iterator> poses(1e1);
            Generate(poses.begin(), poses.end(),
                     [&v] { return v.begin() + std::rand() % v.size(); });

            TVector<int> vCopy = v;

            NthElements(v.begin(), v.end(), poses.begin(), poses.end());
            Sort(vCopy.begin(), vCopy.end());

            for (auto p : poses) {
                UNIT_ASSERT(*p == vCopy[p - v.begin()]);
            }
        }

        {
            TVector<int> v(1e2, 0);
            TVector<TVector<int>::iterator> poses(1e1);
            Generate(poses.begin(), poses.end(),
                     [&v] { return v.begin() + std::rand() % v.size(); });

            NthElements(v.begin(), v.end(), poses.begin(), poses.end());

            for (auto p : poses) {
                UNIT_ASSERT(*p == 0);
            }
        }
    }

    Y_UNIT_TEST(FastEnough) {
        std::srand(42);
        {
            size_t n = 1000000;
            TVector<int> v(Reserve(n));
            for (size_t i = 0; i < n; ++i) {
                v.push_back(rand() & 1);
            }
            TVector<TVector<int>::iterator> poses = {v.begin() + v.size() / 2};
            NthElements(v.begin(), v.end(), poses.begin(), poses.end());
        }
    }
};
