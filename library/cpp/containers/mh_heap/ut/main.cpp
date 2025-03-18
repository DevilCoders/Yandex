#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include "abstract_merger.h"

class TIterableNumbers {
public:
    using value_type = int;

    TIterableNumbers(std::initializer_list<int> numbers)
        : Numbers(numbers)
        , Iterator(Numbers.begin())
    {
    }

    void Restart() {
        Iterator = Numbers.begin();
    }

    bool Valid() {
        return Iterator != Numbers.end();
    }

    TIterableNumbers& operator++() {
        ++Iterator;
        return *this;
    }

    int Current() const {
        return *Iterator;
    }

private:
    TVector<int> Numbers;
    TVector<int>::iterator Iterator;
};

template <typename Merger>
TVector<int> TestEmptyMerger(Merger&) {
    return {};
}

template <typename Merger>
TVector<int> TestSingleIterable(Merger& merger) {
    merger.CreateIterable(std::initializer_list<int>{1, 2, 20, 30, 60});
    return {1, 2, 20, 30, 60};
}

template <typename Merger>
TVector<int> TestTwoIterables(Merger& merger) {
    merger.CreateIterable(std::initializer_list<int>{1, 2, 20, 30, 60});
    merger.CreateIterable(std::initializer_list<int>{2, 3, 4, 5, 40, 70});
    return {1, 2, 2, 3, 4, 5, 20, 30, 40, 60, 70};
}

template <typename Merger>
TVector<int> TestMultipleIterables(Merger& merger) {
    merger.CreateIterable(std::initializer_list<int>{1, 2, 20, 30, 60});
    merger.CreateIterable(std::initializer_list<int>{2, 3, 4, 5, 40, 70});
    merger.CreateIterable(std::initializer_list<int>{2, 70, 80});
    return {1, 2, 2, 2, 3, 4, 5, 20, 30, 40, 60, 70, 70, 80};
}

template <typename InitCallback>
void runTest(InitCallback&& initCallback) {
    TAbstractMerger<TIterableNumbers> merger;

    TVector<int> expected = initCallback(merger);
    merger.Restart();

    TVector<int> actual;
    while (merger.Valid()) {
        actual.push_back(merger.Current().Current());
        ++merger;
    }

    UNIT_ASSERT_VALUES_EQUAL(actual, expected);
}

Y_UNIT_TEST_SUITE(MergerTestSuite) {
    Y_UNIT_TEST(TestEmptyMerger) {
        runTest([](auto& merger) {
            return TestEmptyMerger(merger);
        });
    }
    Y_UNIT_TEST(TestSingleIterable) {
        runTest([](auto& merger) {
            return TestSingleIterable(merger);
        });
    }
    Y_UNIT_TEST(TestTwoIterables) {
        runTest([](auto& merger) {
            return TestTwoIterables(merger);
        });
    }
    Y_UNIT_TEST(TestMultipleIterables) {
        runTest([](auto& merger) {
            return TestMultipleIterables(merger);
        });
    }
}
