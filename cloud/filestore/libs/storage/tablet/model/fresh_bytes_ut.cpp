#include "fresh_bytes.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TFreshBytesVisitor final
    : public IFreshBytesVisitor
{
    TVector<TBytes> Bytes;
    TString Data;

    bool InsertBreaks = true;

    void Accept(const TBytes& bytes, TStringBuf data) override
    {
        Bytes.push_back(bytes);
        if (InsertBreaks && Data) {
            Data += "|";
        }
        Data += data;
    }
};

////////////////////////////////////////////////////////////////////////////////

TString GenerateData(ui32 len)
{
    TString data(len, 0);
    for (ui32 i = 0; i < len; ++i) {
        data[i] = 'a' + (i % ('z' - 'a' + 1));
    }
    return data;
}

////////////////////////////////////////////////////////////////////////////////

#define COMPARE_BYTES(expected, actual)                                        \
    for (ui32 i = 0; i < Max(expected.size(), actual.size()); ++i) {           \
        TString actualStr = "(none)";                                          \
        if (i < actual.size()) {                                               \
            actualStr = actual[i].Describe();                                  \
        }                                                                      \
        TString expectedStr = "(none)";                                        \
        if (i < expectedStr.size()) {                                          \
            expectedStr = expected[i].Describe();                              \
        }                                                                      \
        UNIT_ASSERT_VALUES_EQUAL(expectedStr, actualStr);                      \
    }                                                                          \
// COMPARE_BYTES

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TFreshBytesTest)
{
    Y_UNIT_TEST(ShouldStoreBytes)
    {
        TFreshBytes freshBytes;

        freshBytes.AddBytes(1, 100, "aAa", 10);
        freshBytes.AddBytes(1, 101, "bBbB", 11);
        freshBytes.AddBytes(1, 50, "cCc", 12);
        freshBytes.AddBytes(1, 50, "dDd", 13);
        freshBytes.AddBytes(2, 100, "eEeEe", 14);
        freshBytes.AddBytes(2, 1000, "fFf", 15);
        freshBytes.AddDeletionMarker(2, 100, 3, 16);

        {
            TFreshBytesVisitor visitor;
            freshBytes.FindBytes(visitor, 1, TByteRange(0, 1000, 4_KB), 15);

            COMPARE_BYTES(
                TVector<TBytes>({
                    {1, 50, 3, 13, InvalidCommitId},
                    {1, 100, 1, 10, InvalidCommitId},
                    {1, 101, 4, 11, InvalidCommitId},
                }), visitor.Bytes);

            UNIT_ASSERT_VALUES_EQUAL("dDd|a|bBbB", visitor.Data);
        }

        {
            TFreshBytesVisitor visitor;
            freshBytes.FindBytes(visitor, 2, TByteRange(0, 1000, 4_KB), 15);

            COMPARE_BYTES(
                TVector<TBytes>({
                    {2, 103, 2, 14, InvalidCommitId},
                }), visitor.Bytes);

            UNIT_ASSERT_VALUES_EQUAL("Ee", visitor.Data);
        }

        TVector<TBytes> bytes;
        auto info = freshBytes.StartCleanup(17, &bytes);

        COMPARE_BYTES(
            TVector<TBytes>({
                {1, 100, 3, 10, InvalidCommitId},
                {1, 101, 4, 11, InvalidCommitId},
                {1, 50, 3, 12, InvalidCommitId},
                {1, 50, 3, 13, InvalidCommitId},
                {2, 100, 5, 14, InvalidCommitId},
                {2, 1000, 3, 15, InvalidCommitId},
            }), bytes);
        UNIT_ASSERT_VALUES_EQUAL(17, info.ClosingCommitId);

        freshBytes.FinishCleanup(info.ChunkId);

        {
            TFreshBytesVisitor visitor;
            freshBytes.FindBytes(visitor, 1, TByteRange(0, 1000, 4_KB), 14);
            COMPARE_BYTES(TVector<TBytes>(), visitor.Bytes);
            UNIT_ASSERT_VALUES_EQUAL(TString(), visitor.Data);
        }

        {
            TFreshBytesVisitor visitor;
            freshBytes.FindBytes(visitor, 2, TByteRange(0, 1000, 4_KB), 14);
            COMPARE_BYTES(TVector<TBytes>(), visitor.Bytes);
            UNIT_ASSERT_VALUES_EQUAL(TString(), visitor.Data);
        }
    }

    Y_UNIT_TEST(ShouldInsertIntervalInTheMiddleOfAnotherInterval)
    {
        TFreshBytes freshBytes;

        TString data = GenerateData(1663);

        freshBytes.AddBytes(1, 0, data, 10);
        freshBytes.AddBytes(1, 4, "1234", 11);

        {
            TFreshBytesVisitor visitor;
            visitor.InsertBreaks = false;
            freshBytes.FindBytes(visitor, 1, TByteRange(0, 4_KB, 4_KB), 11);

            COMPARE_BYTES(
                TVector<TBytes>({
                    {1, 0, 4, 10, InvalidCommitId},
                    {1, 4, 4, 11, InvalidCommitId},
                    {1, 8, 1655, 10, InvalidCommitId},
                }), visitor.Bytes);

            TString expected = data;
            expected[4] = '1';
            expected[5] = '2';
            expected[6] = '3';
            expected[7] = '4';

            UNIT_ASSERT_VALUES_EQUAL(expected, visitor.Data);
        }
    }

    // TODO test all branches of AddBytes

    // TODO test with multiple chunks
}

}   // namespace NCloud::NFileStore::NStorage
