#include <util/generic/yexception.h>

#include <library/cpp/testing/unittest/registar.h>

#include "remaptable.h"

using namespace NM2N;

namespace {

    class TMemoryRemapSource {
        struct TFromToRec {
            ui16 FromCl;
            ui32 FromDocId;
            ui16 ToCl;
            ui32 ToDocId;
        };
        TVector<TFromToRec> Records;
        mutable ui32 Index;
    public:
        TMemoryRemapSource() {
            Restart();
        }
        void Push(ui16 fromCl, ui32 fromDocId, ui16 toCl, ui32 toDocId) {
            const TFromToRec rec = { fromCl, fromDocId, toCl, toDocId };
            Records.push_back(rec);
        }
        void Next() const {
            if (IsValid())
                ++Index;
        }
        bool IsValid() const {
            return (Index < Records.size());
        }
        void Restart() {
            Index = 0;
        }
        ui16 GetFromCl() const {
            Y_ASSERT(IsValid());
            return Records[Index].FromCl;
        }
        ui32 GetFromDocId() const {
            Y_ASSERT(IsValid());
            return Records[Index].FromDocId;
        }
        ui16 GetToCl() const {
            Y_ASSERT(IsValid());
            return Records[Index].ToCl;
        }
        ui32 GetToDocId() const {
            Y_ASSERT(IsValid());
            return Records[Index].ToDocId;
        }
    };

    typedef TForwardMultipleRemapTableImpl<TMemoryRemapSource> TForwardMultipleRemapTable;

}

class TRobotRemapTableTest : public TTestBase {
    UNIT_TEST_SUITE(TRobotRemapTableTest);
        UNIT_TEST(TestRemapTable1);
        UNIT_TEST(TestRemapTable2);
        UNIT_TEST(TestRemapTable3);
        UNIT_TEST(TestCompactRemapTable1);
        UNIT_TEST(TestCompactRemapTable2);
        UNIT_TEST(TestMultipleRemapTable1);
        UNIT_TEST(TestMultipleRemapTable2);
        UNIT_TEST(TestCompactRemapTable3);
        UNIT_TEST(TestCompactRemapTable4);
        UNIT_TEST(TestInvertRemapTable1);
        UNIT_TEST(TestInvertRemapTable2);
    UNIT_TEST_SUITE_END();

private:
    template<typename TRemapTableClass>
    void TestInvertRemapTableImpl();

public:
    void TestRemapTable1();
    void TestRemapTable2();
    void TestRemapTable3();
    void TestCompactRemapTable1();
    void TestCompactRemapTable2();
    void TestMultipleRemapTable1();
    void TestMultipleRemapTable2();
    void TestCompactRemapTable3();
    void TestCompactRemapTable4();
    void TestInvertRemapTable1();
    void TestInvertRemapTable2();
};

static void CreateTestTable(TMemoryRemapSource& remapSource) {
    remapSource.Push(0, 1, 0, 1);
    remapSource.Push(0, 2, 2, 0);
    remapSource.Push(1, 0, 0, 2);
    remapSource.Push(1, 1, 1, 2);
    remapSource.Push(3, 3, 4, 4);
    remapSource.Push(3, 4, 4, 4);
}

static void CreateMultipleTestTable(TMemoryRemapSource& remapSource) {
    remapSource.Push(1, 1, 0, 1);
    remapSource.Push(1, 1, 1, 1);
    remapSource.Push(1, 1, 2, 2);
    remapSource.Push(1, 2, 2, 0);
    remapSource.Push(2, 0, 0, 2);
    remapSource.Push(2, 1, 0, 1);
}

template<typename TRemapTableClass>
static void CheckRemapTable(const TRemapTableClass& remap) {
    UNIT_ASSERT(remap.DstClustersCount() == 5);
    UNIT_ASSERT(remap.SrcClustersCount() == 4);
    UNIT_ASSERT(remap.GetSrcMaxDocId(0) == 2);
    UNIT_ASSERT(remap.GetSrcMaxDocId(1) == 1);
    UNIT_ASSERT(remap.GetSrcMaxDocId(2) == Max<ui32>());
    UNIT_ASSERT(remap.GetSrcMaxDocId(3) == 4);
    UNIT_ASSERT(remap.GetDstMaxDocId(0) == 2);
    UNIT_ASSERT(remap.GetDstMaxDocId(1) == 2);
    UNIT_ASSERT(remap.GetDstMaxDocId(2) == 0);
    UNIT_ASSERT(remap.GetDstMaxDocId(3) == Max<ui32>());
    UNIT_ASSERT(remap.GetDstMaxDocId(4) == 4);
    UNIT_ASSERT(!remap.IsEmpty());

    ui32 dstDocId = 0;
    ui16 dstClId = 0;

    UNIT_ASSERT(!remap.GetDst(0, 0, &dstDocId, &dstClId));

    UNIT_ASSERT(remap.GetDst(2, 0, &dstDocId, &dstClId));
    UNIT_ASSERT(dstDocId == 0 && dstClId == 2);

    UNIT_ASSERT(!remap.GetDst(25, 0, &dstDocId, &dstClId));

    UNIT_ASSERT(remap.GetDst(0, 1, &dstDocId, &dstClId));
    UNIT_ASSERT(dstDocId == 2 && dstClId == 0);

    UNIT_ASSERT(remap.GetDst(1, 1, &dstDocId, &dstClId));
    UNIT_ASSERT(dstDocId == 2 && dstClId == 1);

    UNIT_ASSERT(remap.GetDst(3, 3, &dstDocId, &dstClId));
    UNIT_ASSERT(dstDocId == 4 && dstClId == 4);

    UNIT_ASSERT(remap.GetDst(4, 3, &dstDocId, &dstClId));
    UNIT_ASSERT(dstDocId == 4 && dstClId == 4);

    UNIT_ASSERT(!remap.GetDst(1, 4, &dstDocId, &dstClId));
}

template<typename TRemapTableClass>
void CreateAndLoadRemapTable(TRemapTableClass* remap) {
    TMemoryRemapSource remapSource;
    CreateTestTable(remapSource);

    UNIT_ASSERT(remap->IsEmpty());

    LoadRemapTableFromSource(*remap, remapSource, true);
}

void TRobotRemapTableTest::TestRemapTable1() {
    TRemapTable remap;
    CreateAndLoadRemapTable(&remap);

    CheckRemapTable(remap);
}

void TRobotRemapTableTest::TestRemapTable2() {
    TMultipleRemapTable remap;
    CreateAndLoadRemapTable(&remap);

    CheckRemapTable(remap);
}

void TRobotRemapTableTest::TestRemapTable3() {
    TMemoryRemapSource remapSource;
    CreateTestTable(remapSource);

    TForwardMultipleRemapTable remap(remapSource);

    CheckRemapTable(remap);
}

void TRobotRemapTableTest::TestCompactRemapTable1() {
    TCompactMultipleRemapTable compactRemap;
    CreateAndLoadRemapTable(&compactRemap);

    CheckRemapTable(compactRemap);
}

void TRobotRemapTableTest::TestCompactRemapTable2() {
    TMultipleRemapTable remap;
    CreateAndLoadRemapTable(&remap);

    TCompactMultipleRemapTable compactRemap;
    FillCompactRemapTable(remap, &compactRemap);
    compactRemap.FreeUnusedMemory();

    CheckRemapTable(compactRemap);
}

template<typename TRemapTableClass>
static void CheckMultipleRemapTable(const TRemapTableClass& remap, bool isForwardOnly=false) {
    UNIT_ASSERT(remap.DstClustersCount() == 3);
    UNIT_ASSERT(remap.SrcClustersCount() == 3);
    UNIT_ASSERT(remap.GetSrcMaxDocId(0) == Max<ui32>());
    UNIT_ASSERT(remap.GetSrcMaxDocId(1) == 2);
    UNIT_ASSERT(remap.GetSrcMaxDocId(2) == 1);
    UNIT_ASSERT(remap.GetDstMaxDocId(0) == 2);
    UNIT_ASSERT(remap.GetDstMaxDocId(1) == 1);
    UNIT_ASSERT(remap.GetDstMaxDocId(2) == 2);
    UNIT_ASSERT(!remap.IsEmpty());

    ui32 dstDocId;
    ui16 dstClId;

    TVector<TDocCl> dsts;

    {
        dsts.push_back(TDocCl(7, 8)); // Check that GetMultipleDst clears result before filling it
        UNIT_ASSERT(remap.GetMultipleDst(1, 1, &dsts));

        TVector<TDocCl> expected;
        expected.push_back(TDocCl(1, 0));
        expected.push_back(TDocCl(1, 1));
        expected.push_back(TDocCl(2, 2));

        UNIT_ASSERT(dsts.size() == expected.size());

        for (ui32 i = 0; i < dsts.size(); ++i) {
            UNIT_ASSERT(dsts[i].DocId == expected[i].DocId);
            UNIT_ASSERT(dsts[i].ClId == expected[i].ClId);
        }
    }

    if (!isForwardOnly) {
        bool failed = false;
        try {
            remap.GetDst(1, 1, &dstDocId, &dstClId);
        } catch (...) {
            failed = true;
        }

        UNIT_ASSERT(failed);
    }

    UNIT_ASSERT(remap.GetDst(0, 2, &dstDocId, &dstClId));
    UNIT_ASSERT(dstDocId == 2 && dstClId == 0);

    if (!isForwardOnly) {
        UNIT_ASSERT(!remap.GetDst(1, 3, &dstDocId, &dstClId));
    }

    UNIT_ASSERT(!remap.GetMultipleDst(1, 3, &dsts));
}

template<typename TRemapTableClass>
static void CreateAndLoadLoadMultipleRemapTable(TRemapTableClass* remap) {
    TMemoryRemapSource remapSource;
    CreateMultipleTestTable(remapSource);

    UNIT_ASSERT(remap->IsEmpty());

    LoadRemapTableFromSource(*remap, remapSource, true);
}

void TRobotRemapTableTest::TestMultipleRemapTable1() {
    TMultipleRemapTable remap;
    CreateAndLoadLoadMultipleRemapTable(&remap);

    CheckMultipleRemapTable(remap);
}

void TRobotRemapTableTest::TestMultipleRemapTable2() {
    TMemoryRemapSource remapSource;
    CreateMultipleTestTable(remapSource);

    TForwardMultipleRemapTable remap(remapSource);

    CheckMultipleRemapTable(remap, true);
}

void TRobotRemapTableTest::TestCompactRemapTable3() {
    TCompactMultipleRemapTable remap;
    CreateAndLoadLoadMultipleRemapTable(&remap);

    CheckMultipleRemapTable(remap);
}

void TRobotRemapTableTest::TestCompactRemapTable4() {
    TMultipleRemapTable remap;
    CreateAndLoadLoadMultipleRemapTable(&remap);

    TCompactMultipleRemapTable compactRemap;

    FillCompactRemapTable(remap, &compactRemap);
    compactRemap.FreeUnusedMemory();

    CheckMultipleRemapTable(compactRemap);
}

template<typename TRemapTableClass>
static void LoadRemapTableAndDoubleInvert(TMemoryRemapSource& remapSource, TRemapTableClass* doubleInvertedRemap) {
    TRemapTableClass remap;
    LoadRemapTableFromSource(remap, remapSource, true);

    TRemapTableClass invertedRemap;

    InvertRemapTable(remap, &invertedRemap);
    InvertRemapTable(invertedRemap, doubleInvertedRemap);
}

template<typename TRemapTableClass>
void TRobotRemapTableTest::TestInvertRemapTableImpl() {
    TMemoryRemapSource remapSource;
    CreateTestTable(remapSource);

    TRemapTableClass doubleInvertedRemap;

    LoadRemapTableAndDoubleInvert<TRemapTableClass>(remapSource, &doubleInvertedRemap);

    CheckRemapTable(doubleInvertedRemap);
}

void TRobotRemapTableTest::TestInvertRemapTable1() {
    TestInvertRemapTableImpl<TMultipleRemapTable>();
}

void TRobotRemapTableTest::TestInvertRemapTable2() {
    TMemoryRemapSource remapSource;
    CreateMultipleTestTable(remapSource);

    TMultipleRemapTable doubleInvertedRemap;

    LoadRemapTableAndDoubleInvert<TMultipleRemapTable>(remapSource, &doubleInvertedRemap);

    CheckMultipleRemapTable(doubleInvertedRemap);
}

UNIT_TEST_SUITE_REGISTRATION(TRobotRemapTableTest);
