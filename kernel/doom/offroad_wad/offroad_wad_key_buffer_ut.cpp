#include <library/cpp/testing/unittest/registar.h>

#include "offroad_wad_key_buffer.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TOffroadWadKeyBufferTest) {

    using TKeyBuffer = TOffroadKeyBuffer<TString>;

    Y_UNIT_TEST(TestSortedInputNoFrequencySort) {
        TKeyBuffer buffer;
        buffer.Reset();

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("aba");

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("abacaba");

        buffer.AddHit();
        buffer.AddKey("bacadaba");

        buffer.AddKey("hello");

        buffer.PrepareKeys(false);

        TDeque<TString> expectedKeys = { "aba", "abacaba", "bacadaba", "hello" };
        UNIT_ASSERT_VALUES_EQUAL(expectedKeys, buffer.Keys());
        TVector<ui32> expectedMapping = { 0, 1, 2, 3 };
        UNIT_ASSERT_VALUES_EQUAL(expectedMapping, buffer.KeysMapping());
        UNIT_ASSERT_VALUES_EQUAL(expectedMapping, buffer.SortedKeysMapping());
    }

    Y_UNIT_TEST(TestUnsortedInputNoFrequencySort) {
        TKeyBuffer buffer;
        buffer.Reset();

        buffer.AddKey("hello");

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("abacaba");

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("aba");

        buffer.AddHit();
        buffer.AddKey("bacadaba");

        buffer.PrepareKeys(false);

        TDeque<TString> expectedKeys = { "aba", "abacaba", "bacadaba", "hello" };
        UNIT_ASSERT_VALUES_EQUAL(expectedKeys, buffer.Keys());
        TVector<ui32> expectedMapping = { 3, 1, 0, 2 };
        UNIT_ASSERT_VALUES_EQUAL(expectedMapping, buffer.KeysMapping());
        TVector<ui32> expectedSortedMapping = { 0, 1, 2, 3 };
        UNIT_ASSERT_VALUES_EQUAL(expectedSortedMapping, buffer.SortedKeysMapping());
    }

    Y_UNIT_TEST(TestSortedInputFrequencySort) {
        TKeyBuffer buffer;
        buffer.Reset();

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("aba");

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("abacaba");

        buffer.AddHit();
        buffer.AddKey("bacadaba");

        buffer.AddKey("hello");

        buffer.PrepareKeys(true);

        TDeque<TString> expectedKeys = { "aba", "abacaba", "bacadaba", "hello" };
        UNIT_ASSERT_VALUES_EQUAL(expectedKeys, buffer.Keys());
        TVector<ui32> expectedMapping = { 1, 0, 2, 3 };
        UNIT_ASSERT_VALUES_EQUAL(expectedMapping, buffer.KeysMapping());
        TVector<ui32> expectedSortedMapping = { 1, 0, 2, 3 };
        UNIT_ASSERT_VALUES_EQUAL(expectedSortedMapping, buffer.SortedKeysMapping());
    }

    Y_UNIT_TEST(TestUnsortedInputFrequencySort) {
        TKeyBuffer buffer;
        buffer.Reset();

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("hello");

        buffer.AddHit();
        buffer.AddKey("abacaba");

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("aba");

        buffer.AddHit();
        buffer.AddHit();
        buffer.AddKey("bacadaba");

        buffer.PrepareKeys(true);

        TDeque<TString> expectedKeys = { "aba", "abacaba", "bacadaba", "hello" };
        UNIT_ASSERT_VALUES_EQUAL(expectedKeys, buffer.Keys());
        TVector<ui32> expectedMapping = { 0, 3, 1, 2 };
        UNIT_ASSERT_VALUES_EQUAL(expectedMapping, buffer.KeysMapping());
        TVector<ui32> expectedSortedMapping = { 1, 3, 2, 0 };
        UNIT_ASSERT_VALUES_EQUAL(expectedSortedMapping, buffer.SortedKeysMapping());
    }
}
