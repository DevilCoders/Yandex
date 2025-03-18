#include <util/random/shuffle.h>
#include <iterator>
#include <algorithm>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include "reverse.h"
#include "sort.h"

Y_UNIT_TEST_SUITE(TestExt) {
    template <class TExtAlgo, class TSimpleAlgo, class TIter>
    void Check(TIter begin, TIter end, TSimpleAlgo algo, size_t pagesize) {
        typedef typename std::iterator_traits<TIter>::value_type T;

        TExtAlgo ext(pagesize);
        for (TIter i = begin; i != end; ++i)
            ext.PushBack(*i);

        TVector<T> processed(begin, end);
        algo(processed.begin(), processed.end());

        typename TVector<T>::iterator i = processed.begin(), ie = processed.end();

        // Next() should work without preceeding Rewind()
        const typename TExtAlgo::TValue* rec = nullptr;
        while ((rec = ext.Next())) {
            if (ext.Current() != *i)
                throw yexception() << "On item #" << std::distance(processed.begin(), i) << ": ext="
                                   << ext.Current() << " mismatches int=" << *i;
            ++i;
        }

        UNIT_ASSERT(i == ie);
        UNIT_ASSERT(ext.AtEnd());

        // Next() should work properly after Rewind()
        ext.Rewind();
        rec = nullptr;
        i = processed.begin();
        while ((rec = ext.Next())) {
            if (ext.Current() != *i)
                throw yexception() << "On item #" << std::distance(processed.begin(), i) << ": ext="
                                   << ext.Current() << " mismatches int=" << *i;
            ++i;
        }

        UNIT_ASSERT(i == ie);
        UNIT_ASSERT(ext.AtEnd());

        ext.Rewind();
        i = processed.begin();
        for (; !ext.AtEnd() && i != ie; ext.Advance(), ++i)
            if (ext.Current() != *i)
                throw yexception() << "On item #" << std::distance(processed.begin(), i) << ": ext="
                                   << ext.Current() << " mismatches int=" << *i;

        UNIT_ASSERT(i == ie);
        UNIT_ASSERT(ext.AtEnd());
    }

    struct TFixture {
        TVector<TString> ShortStrings;
        TVector<TString> LongStrings;
        TVector<int> Ints;

        template <class T>
        static void Sort(typename TVector<T>::iterator begin, typename TVector<T>::iterator end) {
            std::sort(begin, end);
        }

        static void Reverse(TVector<TString>::iterator begin, TVector<TString>::iterator end) {
            std::reverse(begin, end);
        }

        TFixture() {
            for (int i = 0; i < 10000; ++i)
                ShortStrings.push_back(Sprintf("This is a test string #%04d", i));

            TString longstr = "How long is LongString? LongString is lo";
            for (int i = 0; i < 10000; ++i)
                longstr.append('o');
            longstr.append("ng!");

            for (int i = 0; i < 100; ++i)
                LongStrings.push_back(Sprintf("%s #%03d", longstr.data(), i));

            for (int i = 0; i < 1000000; ++i)
                Ints.push_back(i);

            srand(0);
            Shuffle(ShortStrings.begin(), ShortStrings.end());
            Shuffle(LongStrings.begin(), LongStrings.end());
            Shuffle(Ints.begin(), Ints.end());
        }
    } fixture;

    Y_UNIT_TEST(Reverse) {
        Check<TExtReverse<TString>>(fixture.ShortStrings.begin(), fixture.ShortStrings.end(),
                                    &TFixture::Reverse, 1024 * 1024);
    }

    Y_UNIT_TEST(ReverseSmallPage) {
        Check<TExtReverse<TString>>(fixture.ShortStrings.begin(), fixture.ShortStrings.end(),
                                    &TFixture::Reverse, 10240);
    }

    Y_UNIT_TEST(ReverseLongItems) {
        Check<TExtReverse<TString>>(fixture.LongStrings.begin(), fixture.LongStrings.end(),
                                    &TFixture::Reverse, 10240);
    }

    Y_UNIT_TEST(Sort) {
        Check<TExtSort<TString>>(fixture.ShortStrings.begin(), fixture.ShortStrings.end(),
                                 &TFixture::Sort<TString>, 1024 * 1024);
    }

    Y_UNIT_TEST(SortEmpty) {
        Check<TExtSort<TString>>(fixture.ShortStrings.begin(), fixture.ShortStrings.begin(),
                                 &TFixture::Sort<TString>, 1024 * 1024);
    }

    Y_UNIT_TEST(SortSmallPage) {
        Check<TExtSort<TString>>(fixture.ShortStrings.begin(), fixture.ShortStrings.end(),
                                 &TFixture::Sort<TString>, 10240);
    }

    Y_UNIT_TEST(SortLongItems) {
        Check<TExtSort<TString>>(fixture.LongStrings.begin(), fixture.LongStrings.end(),
                                 &TFixture::Sort<TString>, 10240);
    }

    Y_UNIT_TEST(SortInts) {
        Check<TExtSort<int>>(fixture.Ints.begin(), fixture.Ints.end(), &TFixture::Sort<int>, 100000);
    }
}
