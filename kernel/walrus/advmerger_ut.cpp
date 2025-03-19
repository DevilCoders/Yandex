#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/digest/old_crc/crc.h>

#include <kernel/keyinv/indexfile/memoryportion.h>
#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/indexfile/indexstoragefactory.h>
#include <kernel/indexer/directindex/directindex.h>
#include <kernel/indexer/posindex/invcreator.h>
#include <kernel/walrus/advmerger.h>
#include <ysite/directtext/freqs/freqs.h>

//#ifdef UNIT_ASSERT_STRINGS_EQUAL
//#undef UNIT_ASSERT_STRINGS_EQUAL
//#endif
//#define UNIT_ASSERT_STRINGS_EQUAL(a, b) Cout << "    UNIT_ASSERT_STRINGS_EQUAL(\"" << b << "\", " << #b << ");" << Endl;

class TAdvancedMergeTest: public TTestBase
{
    UNIT_TEST_SUITE(TAdvancedMergeTest);
        UNIT_TEST(TestM2NSort);
        UNIT_TEST(TestMerge);
        UNIT_TEST(TestOnlyLemmas);
        UNIT_TEST(TestRawKeys);
        UNIT_TEST(TestMemoryPortions);
        UNIT_TEST(TestMemoryPortionsWithVersion);
        UNIT_TEST(TestOutputMemoryPortion);
        UNIT_TEST(TestOutputIntersectedMemoryPortions)
    UNIT_TEST_SUITE_END();
private:
    void TestMemoryPortionsImpl(NIndexerCore::TMemoryPortionFactory& memoryPortionFactory);
public:
    void TestM2NSort();
    void TestMerge();
    void TestOnlyLemmas();
    void TestRawKeys();
    void TestMemoryPortions();
    void TestMemoryPortionsWithVersion();
    void TestOutputMemoryPortion();
    void TestOutputIntersectedMemoryPortions();
};

namespace {

    void MakePortion1(IYndexStorageFactory& storage, ui32& docID, const NIndexerCore::TInvCreatorConfig& invCfg) {
        NIndexerCore::TDTCreatorConfig dtcCfg;
        NIndexerCore::TDirectTextCreator dtc(dtcCfg, TLangMask(LANG_RUS, LANG_ENG), LANG_RUS);
        NIndexerCore::TInvCreatorDTCallback ic(&storage, invCfg);
        TFreqCalculator freqCalculator(false);
        NIndexerCore::TDirectIndex index(dtc);
        index.SetInvCreator(&ic);
        index.AddDirectTextCallback(&freqCalculator);

        const TUtf16String text1 = UTF8ToWide("поближе, ближе, близкой, близко, ближайшее, ближайшие. Близкий, близком, близкие, ближайшего, ближайший. "
            "Близко, близким. Ближайшие, близких, близкими.");
        const TUtf16String text2 = u"близок, ближайшем, ближайших, ближайшую, ближайшему."; // these forms will move to the lemma with the most frequent forms in the result index

        index.AddDoc(docID++, LANG_RUS);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc00"));
        index.StoreText(text1.c_str(), text1.size(), MID_RELEV);
        index.StoreText(text1.c_str(), text1.size(), MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(docID++, LANG_RUS);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc01"));
        index.StoreText(text1.c_str(), text1.size(), MID_RELEV);
        index.StoreText(text2.c_str(), text2.size(), MID_RELEV);
        index.StoreText(text2.c_str(), text2.size(), MID_RELEV);
        index.CommitDoc(nullptr, nullptr);
    }

    void MakePortion2(IYndexStorageFactory& storage, ui32& docID, const NIndexerCore::TInvCreatorConfig& invCfg) {
        NIndexerCore::TDTCreatorConfig dtcCfg;
        NIndexerCore::TDirectTextCreator dtc(dtcCfg, TLangMask(LANG_RUS, LANG_ENG), LANG_RUS);
        NIndexerCore::TInvCreatorDTCallback ic(&storage, invCfg);
        TFreqCalculator freqCalculator(false);
        NIndexerCore::TDirectIndex index(dtc);
        index.SetInvCreator(&ic);
        index.AddDirectTextCallback(&freqCalculator);

        const TString text1 = "00001 000001 0000001 00000001 000000001 0000000001 00000000001 000000000001 0000000000001 "
            "00000000000001 000000000000001 0000000000000001 00000000000000001 000000000000000001 0000000000000000001 00000000000000000001.";
        const TString text2 = "1 01 001 0001"; // these forms must be moved to the lemma "1" with the most frequent forms in the result index

        index.AddDoc(docID++, LANG_RUS);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc10"));
        index.StoreUtf8Text(text1, MID_RELEV);
        index.StoreUtf8Text(text1, MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(docID++, LANG_RUS);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc11"));
        index.StoreUtf8Text(text1, MID_RELEV);
        index.StoreUtf8Text(text2, MID_RELEV);
        index.StoreUtf8Text(text2, MID_RELEV);
        index.CommitDoc(nullptr, nullptr);
    }

    void MakePortion3(IYndexStorageFactory& storage, ui32& docID, const NIndexerCore::TInvCreatorConfig& invCfg) {
        NIndexerCore::TDTCreatorConfig dtcCfg;
        NIndexerCore::TDirectTextCreator dtc(dtcCfg, TLangMask(LANG_RUS, LANG_ENG), LANG_RUS);
        NIndexerCore::TInvCreatorDTCallback ic(&storage, invCfg);
        TFreqCalculator freqCalculator(false);
        NIndexerCore::TDirectIndex index(dtc);
        index.SetInvCreator(&ic);
        index.AddDirectTextCallback(&freqCalculator);

        // 00000000001 LR (1- R, 01. LR, 1. LR, 1 L, 1@ R, 01- LR, 1, 1- LR, 1. R, 01/ LR, 001- LR, 1/ LR, 1/ R, 1_ R, 001- R, 01- R) 7714
        const TString text1 = "a1-b, a-01.b, a-1.b, a-1b, a1@b, a-01-b, a1b, a-1-b, a1.b, a/01/b, a/001-b, a/1/b, a1/b, a1_b, a001-b, a01-b";
        // 00000000001 LR (001 L, 001. LR, 1_ LR, 01. R, 01, 001/ LR, 001. R, 1' R, 01/ R, 001) 369
        const TString text2 = "a-001b, a-001.b, a-1_b, a01.b, a01b, a-001/b, a001.b, a1'b, a01/b, a001b";

        index.AddDoc(docID++, LANG_RUS);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc20"));
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("cat", "1"));
        index.StoreUtf8Text(text1, MID_RELEV);
        index.StoreUtf8Text(text1, MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(docID++, LANG_RUS);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc21"));
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("cat", "1"));
        index.StoreUtf8Text(text1, MID_RELEV);
        index.StoreUtf8Text(text2, MID_RELEV);
        index.StoreUtf8Text(text2, MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(docID++, LANG_RUS);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc22"));
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("cat", "1"));
        index.StoreUtf8Text(text1, MID_RELEV);
        index.StoreUtf8Text(text2, MID_RELEV);
        index.CommitDoc(nullptr, nullptr);
    }

} // namespace

void TAdvancedMergeTest::TestM2NSort() {
    NIndexerCore::TInvCreatorConfig config(50);
    { // index0*
        NIndexerCore::TIndexStorageFactory storage;
        storage.InitIndexResources("index", "./", "index");
        ui32 docID = 0;
        MakePortion1(storage, docID, config);

        {
            TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
            AddInputPortions(*task, storage.GetNames());
            task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index0", YNDEX_VERSION_BLK8));
            TAdvancedIndexMerger merger(task);
            merger.Run();
        }

        storage.RemovePortions();
    }
    { // index1*
        NIndexerCore::TIndexStorageFactory storage;
        storage.InitIndexResources("index", "./", "index");
        ui32 docID = 0;
        MakePortion2(storage, docID, config);

        {
            TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
            AddInputPortions(*task, storage.GetNames());
            task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index1", YNDEX_VERSION_BLK8));
            TAdvancedIndexMerger merger(task);
            merger.Run();
        }

        storage.RemovePortions();
    }
    { // index2*
        NIndexerCore::TIndexStorageFactory storage;
        storage.InitIndexResources("index", "./", "index");
        ui32 docID = 0;
        MakePortion3(storage, docID, config);

        {
            TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
            AddInputPortions(*task, storage.GetNames());
            task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index2", YNDEX_VERSION_BLK8));
            TAdvancedIndexMerger merger(task);
            merger.Run();
        }

        storage.RemovePortions();
    }
    { // m2nsort 3->2
        TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());

        task->Inputs.push_back(TAdvancedMergeTask::TMergeInput("index0"));
        task->Inputs.push_back(TAdvancedMergeTask::TMergeInput("index1"));
        task->Inputs.push_back(TAdvancedMergeTask::TMergeInput("index2"));

        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index3", YNDEX_VERSION_BLK8));
        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index4", YNDEX_VERSION_BLK8));

        task->FinalRemapTable.Create(3, TFinalRemapTable::DELETED_DOCUMENT);
        task->FinalRemapTable.ResizeInputCluster(0, 2); // index0key/index0inv
        task->FinalRemapTable.SetRemapItem(0, 0, 0, 0);
        task->FinalRemapTable.SetRemapItem(0, 1, 1, 0);

        task->FinalRemapTable.ResizeInputCluster(1, 2); // index1key/index1inv
        task->FinalRemapTable.SetRemapItem(1, 0, 1, 1);
        task->FinalRemapTable.SetRemapItem(1, 1, 0, 1);

        task->FinalRemapTable.ResizeInputCluster(2, 3); // index2key/index2inv
        task->FinalRemapTable.SetRemapItem(2, 0, 0, 2);
        task->FinalRemapTable.SetRemapItem(2, 1, 1, 2);
        task->FinalRemapTable.SetRemapItem(2, 2, 0, 3);

        TAdvancedIndexMerger merger(task);
        merger.Run();
    }

    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("864997efa5e30878c3cf945c05a1ae2b", MD5::File("index0key", buf));
    UNIT_ASSERT_STRINGS_EQUAL("9e4762d90084f355b409665c5e892b84", MD5::File("index0inv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("e9c8aca98f3a0e468da577f4c73d3f41", MD5::File("index1key", buf));
    UNIT_ASSERT_STRINGS_EQUAL("8d63982ca739168e2c14430b1f667702", MD5::File("index1inv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("9781ccd250c4c31db0c2a42178e49238", MD5::File("index2key", buf));
    UNIT_ASSERT_STRINGS_EQUAL("180af846ba2ed64cb9aedf554b203884", MD5::File("index2inv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("d19cb557df27e43ec8696554174f28b2", MD5::File("index3key", buf));
    UNIT_ASSERT_STRINGS_EQUAL("8e2603dac55e70604fe57787df55997f", MD5::File("index3inv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("956586903f374b32a03ae38517b3556c", MD5::File("index4key", buf));
    UNIT_ASSERT_STRINGS_EQUAL("961376878b95287ab59e3a42add03439", MD5::File("index4inv", buf));

    UNIT_ASSERT(unlink("index0key") == 0);
    UNIT_ASSERT(unlink("index0inv") == 0);
    UNIT_ASSERT(unlink("index1key") == 0);
    UNIT_ASSERT(unlink("index1inv") == 0);
    UNIT_ASSERT(unlink("index2key") == 0);
    UNIT_ASSERT(unlink("index2inv") == 0);
    UNIT_ASSERT(unlink("index3key") == 0);
    UNIT_ASSERT(unlink("index3inv") == 0);
    UNIT_ASSERT(unlink("index4key") == 0);
    UNIT_ASSERT(unlink("index4inv") == 0);
}

UNIT_TEST_SUITE_REGISTRATION(TAdvancedMergeTest);

/*
$ bin/printkeys -cz index3key index3inv | iconv -f cp1251 -t utf-8
##_DOC_IDF_SUM 4 18
##_DOC_LENS 4 20
##_HEADING_IDF_SUM 4 7
##_HEADING_LENS 4 7
##_LOW_TEXT_IDF_SUM 4 7
##_LOW_TEXT_LENS 4 7
##_MAX_FREQ 4 20
##_NORMAL_TEXT_IDF_SUM 4 18
##_NORMAL_TEXT_LENS 4 20
##_TITLE_IDF_SUM 4 7
##_TITLE_LENS 4 7
#cat="1 2 5
#url="localhost/directdoc00 1 4
#url="localhost/directdoc11 1 4
#url="localhost/directdoc20 1 4
#url="localhost/directdoc22 1 4
00000000001 (0000000000000001, 000000000000001, 00000000000001, 0000000000001, 000000000001, 00000000001, 0000000001, 000000001, 00000001, 0000001, 000001, 00001, 001 LR, 001 L+R, 01 LR) 15 34
00000000001 (00000000000000000001, 0000000000000000001, 000000000000000001, 00000000000000001, 0001, 001, 001 LR+, 001 L+R+, 01, 01 LR+, 01 L+R+, 1, 1 LR, 1 L+R, 1 LR+, 1 L+R+) 67 114
a {en} (a R, a R+) 58 76
b {en} (b L, b L+) 58 76
ближний {ru} (ближайшего, ближайшее, ближайшие, Ближайшие, ближайший) 10 28
близкие {ru} (близкие, близким, близкими, близких) 8 17
близкий {ru} (ближайшего, ближайшее, ближайшие, Ближайшие, ближайший, ближе, близкие, Близкий, близким, близкими, близких, близко, Близко, близкой, близком, поближе) 32 48
близко {ru} (ближе, близко, Близко, поближе) 8 20

$ bin/printkeys -cz index4key index4inv | iconv -f cp1251 -t utf-8
##_DOC_IDF_SUM 3 17
##_DOC_LENS 3 15
##_HEADING_IDF_SUM 3 6
##_HEADING_LENS 3 6
##_LOW_TEXT_IDF_SUM 3 6
##_LOW_TEXT_LENS 3 6
##_MAX_FREQ 3 13
##_NORMAL_TEXT_IDF_SUM 3 17
##_NORMAL_TEXT_LENS 3 15
##_TITLE_IDF_SUM 3 6
##_TITLE_LENS 3 6
#cat="1 1 4
#url="localhost/directdoc01 1 4
#url="localhost/directdoc10 1 4
#url="localhost/directdoc21 1 4
00000000001 (0000000001, 000000001, 00000001, 0000001, 000001, 00001, 001 LR, 001 L+R, 01 LR, 1 LR, 1 L+R) 20 44
00000000001 (00000000000000000001, 0000000000000000001, 000000000000000001, 00000000000000001, 0000000000000001, 000000000000001, 00000000000001, 0000000000001, 000000000001, 00000000001, 001 LR+, 001 L+R+, 01 LR+, 01 L+R+, 1 LR+, 1 L+R+) 48 75
a {en} (a R, a R+) 36 50
b {en} (b L, b L+) 36 51
ближний {ru} (ближайшего, ближайшее, ближайшем, ближайшему, ближайшие, Ближайшие, ближайший, ближайших, ближайшую) 13 29
близкие {ru} (близкие, близким, близкими, близких) 4 12
близкий {ru} (близко, Близко, близкой, близком, поближе) 5 15
близкий {ru} (ближайшего, ближайшее, ближайшем, ближайшему, ближайшие, Ближайшие, ближайший, ближайших, ближайшую, ближе, близкие, Близкий, близким, близкими, близких, близок) 21 40
близко {ru} (ближе, близко, Близко, поближе) 4 11
*/

void TAdvancedMergeTest::TestMerge() {
    const ELanguage langRus = LANG_RUS;

    { // old index
        NIndexerCore::TIndexStorageFactory storage;
        storage.InitIndexResources("index", "./", "index");
        {
        NIndexerCore::TDTCreatorConfig dtcCfg;
        NIndexerCore::TDirectTextCreator dtc(dtcCfg, TLangMask(LANG_RUS, LANG_ENG), LANG_RUS);
        NIndexerCore::TInvCreatorConfig invCfg(50);
        NIndexerCore::TInvCreatorDTCallback ic(&storage, invCfg);
        TFreqCalculator freqCalculator(false);
        NIndexerCore::TDirectIndex index(dtc);
        index.SetInvCreator(&ic);
        index.AddDirectTextCallback(&freqCalculator);

        index.AddDoc(1, langRus);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc1"));
        index.StoreUtf8Text("A b c. D e f.", MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(2, langRus);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc2"));
        index.StoreUtf8Text("G h i. J k l.", MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(3, langRus);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc3"));
        index.StoreUtf8Text("M n o. P q r.", MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(4, langRus);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc4"));
        index.StoreUtf8Text("S t u. V w x.", MID_RELEV);
        index.CommitDoc(nullptr, nullptr);
        }

        {
            TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
            AddInputPortions(*task, storage.GetNames());
            task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("oldbd", YNDEX_VERSION_BLK8));
            TAdvancedIndexMerger merger(task);
            merger.Run();
        }

        storage.RemovePortions();
    }
    { // portion with reindexed documents (but it will have FINAL_FORMAT)
        NIndexerCore::TIndexStorageFactory storage;
        storage.InitIndexResources("index", "./", "index");
        {
        NIndexerCore::TDTCreatorConfig dtcCfg;
        NIndexerCore::TDirectTextCreator dtc(dtcCfg, TLangMask(LANG_RUS, LANG_ENG), LANG_RUS);
        NIndexerCore::TInvCreatorConfig invCfg(50);
        NIndexerCore::TInvCreatorDTCallback ic(&storage, invCfg);
        TFreqCalculator freqCalculator(false);
        NIndexerCore::TDirectIndex index(dtc);
        index.SetInvCreator(&ic);
        index.AddDirectTextCallback(&freqCalculator);

        index.AddDoc(5, langRus);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc1"));
        index.StoreUtf8Text("A b c d. E f x.", MID_RELEV);
        index.CommitDoc(nullptr, nullptr);

        index.AddDoc(6, langRus);
        index.StoreDocAttr(NIndexerCore::TDirectIndex::TAttribute("url", "localhost/directDoc3"));
        index.StoreUtf8Text("X y o. P q r.", MID_RELEV);
        index.CommitDoc(nullptr, nullptr);
        }

        {
            TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
            AddInputPortions(*task, storage.GetNames());
            task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("portion", YNDEX_VERSION_BLK8));
            TAdvancedIndexMerger merger(task);
            merger.Run();
        }

        storage.RemovePortions();
    }
    {
        TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());

        task->Inputs.push_back(TAdvancedMergeTask::TMergeInput("oldbd"));
        task->Inputs.push_back(TAdvancedMergeTask::TMergeInput("portion")); // but it is FINAL_FORMAT

        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("newbd", YNDEX_VERSION_BLK8));
        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("delta", YNDEX_VERSION_BLK8));
        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("minidocs", YNDEX_VERSION_BLK8));

        // actually size of remap table can be 1 because there are no different documents with the same numbers
        task->FinalRemapTable.Create(2, TFinalRemapTable::DELETED_DOCUMENT);
        task->FinalRemapTable.ResizeInputCluster(0, 5); // index0key/index0inv
        // no document #0
        task->FinalRemapTable.SetDeletedDoc(0, 1);
        task->FinalRemapTable.SetRemapItem(0, 2, 0x80000005, 2);
        task->FinalRemapTable.SetDeletedDoc(0, 3);
        task->FinalRemapTable.SetRemapItem(0, 4, 0, 4);

        task->FinalRemapTable.ResizeInputCluster(1, 7); // index1key/index1inv
        // no documents #0-#4
        // to newbd, delta and minidocs:
        task->FinalRemapTable.SetOutputCluster(1, 5, 0);
        task->FinalRemapTable.SetOutputCluster(1, 5, 1);
        task->FinalRemapTable.SetOutputCluster(1, 5, 2);
        // to newbd and delta:
        task->FinalRemapTable.SetOutputCluster(1, 6, 0);
        task->FinalRemapTable.SetOutputCluster(1, 6, 1);

        TAdvancedIndexMerger merger(task);
        merger.Run();
    }

    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("2b52bbd92628467cc52e18f65aa9939e", MD5::File("oldbdkey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("365437b59aaffc5bb8e2087839d18ba8", MD5::File("oldbdinv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("4de92a3ed3d9bf7d055423bb1f2d80aa", MD5::File("portionkey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("5578fb9aa34131133d6fefcdcc805085", MD5::File("portioninv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("5e12efd8b4b7b11b31a8ae9ed3386973", MD5::File("newbdkey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("d0b2744c81dcd81f3c1d3022e7cc1797", MD5::File("newbdinv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("4de92a3ed3d9bf7d055423bb1f2d80aa", MD5::File("deltakey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("5578fb9aa34131133d6fefcdcc805085", MD5::File("deltainv", buf));
    UNIT_ASSERT_STRINGS_EQUAL("a347d5da9146ee0202dcb5c2c3bf25c3", MD5::File("minidocskey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("cfcd5ff8df9051a04f2b5d0bb83729db", MD5::File("minidocsinv", buf));

    UNIT_ASSERT(unlink("oldbdkey") == 0);
    UNIT_ASSERT(unlink("oldbdinv") == 0);
    UNIT_ASSERT(unlink("portionkey") == 0);
    UNIT_ASSERT(unlink("portioninv") == 0);
    UNIT_ASSERT(unlink("newbdkey") == 0);
    UNIT_ASSERT(unlink("newbdinv") == 0);
    UNIT_ASSERT(unlink("deltakey") == 0);
    UNIT_ASSERT(unlink("deltainv") == 0);
    UNIT_ASSERT(unlink("minidocskey") == 0);
    UNIT_ASSERT(unlink("minidocsinv") == 0);
}
/*
$ bin/printkeys -cfz newbdkey newbdinv
##_DOC_IDF_SUM 4 22
    [2.1128.32.3.0]
    [4.1029.30.2.10]
    [5.919.0.3.14]
    [6.1129.28.1.12]
##_DOC_LENS 4 12
    [2.0.0.0.6]
    [4.0.0.0.6]
    [5.0.0.0.7]
    [6.0.0.0.6]
##_HEADING_IDF_SUM 4 7
    [2.0.0.0.0]
    [4.0.0.0.0]
    [5.0.0.0.0]
    [6.0.0.0.0]
##_HEADING_LENS 4 7
    [2.0.0.0.0]
    [4.0.0.0.0]
    [5.0.0.0.0]
    [6.0.0.0.0]
##_LOW_TEXT_IDF_SUM 4 7
    [2.0.0.0.0]
    [4.0.0.0.0]
    [5.0.0.0.0]
    [6.0.0.0.0]
##_LOW_TEXT_LENS 4 7
    [2.0.0.0.0]
    [4.0.0.0.0]
    [5.0.0.0.0]
    [6.0.0.0.0]
##_MAX_FREQ 4 8
    [2.0.0.0.1]
    [4.0.0.0.1]
    [5.0.0.0.1]
    [6.0.0.0.1]
##_NORMAL_TEXT_IDF_SUM 4 22
    [2.1128.32.3.0]
    [4.1029.30.2.10]
    [5.919.0.3.14]
    [6.1129.28.1.12]
##_NORMAL_TEXT_LENS 4 12
    [2.0.0.0.6]
    [4.0.0.0.6]
    [5.0.0.0.7]
    [6.0.0.0.6]
##_TITLE_IDF_SUM 4 7
    [2.0.0.0.0]
    [4.0.0.0.0]
    [5.0.0.0.0]
    [6.0.0.0.0]
##_TITLE_LENS 4 7
    [2.0.0.0.0]
    [4.0.0.0.0]
    [5.0.0.0.0]
    [6.0.0.0.0]
#url="localhost/directdoc1 1 4
    [5.0.0.0.0]
#url="localhost/directdoc2 1 4
    [2.0.0.0.0]
#url="localhost/directdoc3 1 4
    [6.0.0.0.0]
#url="localhost/directdoc4 1 4
    [4.0.0.0.0]
a {en} (A) 1 6
    [5.1.1.1.0]
b {en} (b) 1 6
    [5.1.2.1.0]
c {en} (c) 1 6
    [5.1.3.1.0]
d {en} (d) 1 6
    [5.1.4.1.0]
e {en} (E) 1 6
    [5.1.5.1.0]
f {en} (f) 1 6
    [5.1.6.1.0]
g {en} (G) 1 6
    [2.1.1.1.0]
h {en} (h) 1 6
    [2.1.2.1.0]
i {en} (i) 1 6
    [2.1.3.1.0]
j {en} (J) 1 6
    [2.1.4.1.0]
k {en} (k) 1 6
    [2.1.5.1.0]
l {en} (l) 1 6
    [2.1.6.1.0]
o {en} (o) 1 6
    [6.1.3.1.0]
p {en} (P) 1 6
    [6.1.4.1.0]
q {en} (q) 1 6
    [6.1.5.1.0]
r {en} (r) 1 6
    [6.1.6.1.0]
s {en} (S) 1 6
    [4.1.1.1.0]
t {en} (t) 1 6
    [4.1.2.1.0]
u {en} (u) 1 6
    [4.1.3.1.0]
v {en} (V) 1 6
    [4.1.4.1.0]
w {en} (w) 1 6
    [4.1.5.1.0]
x {en} (x, X) 3 11
    [4.1.6.1.0]
    [5.1.7.1.0]
    [6.1.1.1.1]
y {en} (y) 1 6
    [6.1.2.1.0]

$ bin/printkeys -cfz deltakey deltainv
##_DOC_IDF_SUM 2 12
    [5.919.0.3.14]
    [6.1129.28.1.12]
##_DOC_LENS 2 8
    [5.0.0.0.7]
    [6.0.0.0.6]
##_HEADING_IDF_SUM 2 5
    [5.0.0.0.0]
    [6.0.0.0.0]
##_HEADING_LENS 2 5
    [5.0.0.0.0]
    [6.0.0.0.0]
##_LOW_TEXT_IDF_SUM 2 5
    [5.0.0.0.0]
    [6.0.0.0.0]
##_LOW_TEXT_LENS 2 5
    [5.0.0.0.0]
    [6.0.0.0.0]
##_MAX_FREQ 2 6
    [5.0.0.0.1]
    [6.0.0.0.1]
##_NORMAL_TEXT_IDF_SUM 2 12
    [5.919.0.3.14]
    [6.1129.28.1.12]
##_NORMAL_TEXT_LENS 2 8
    [5.0.0.0.7]
    [6.0.0.0.6]
##_TITLE_IDF_SUM 2 5
    [5.0.0.0.0]
    [6.0.0.0.0]
##_TITLE_LENS 2 5
    [5.0.0.0.0]
    [6.0.0.0.0]
#url="localhost/directdoc1 1 4
    [5.0.0.0.0]
#url="localhost/directdoc3 1 4
    [6.0.0.0.0]
a {en} (A) 1 6
    [5.1.1.1.0]
b {en} (b) 1 6
    [5.1.2.1.0]
c {en} (c) 1 6
    [5.1.3.1.0]
d {en} (d) 1 6
    [5.1.4.1.0]
e {en} (E) 1 6
    [5.1.5.1.0]
f {en} (f) 1 6
    [5.1.6.1.0]
o {en} (o) 1 6
    [6.1.3.1.0]
p {en} (P) 1 6
    [6.1.4.1.0]
q {en} (q) 1 6
    [6.1.5.1.0]
r {en} (r) 1 6
    [6.1.6.1.0]
x {en} (x, X) 2 9
    [5.1.7.1.0]
    [6.1.1.1.1]
y {en} (y) 1 6
    [6.1.2.1.0]

$ bin/printkeys -cfz minidocskey minidocsinv
##_DOC_IDF_SUM 2 12
    [2.1128.32.3.0]
    [5.919.0.3.14]
##_DOC_LENS 2 8
    [2.0.0.0.6]
    [5.0.0.0.7]
##_HEADING_IDF_SUM 2 5
    [2.0.0.0.0]
    [5.0.0.0.0]
##_HEADING_LENS 2 5
    [2.0.0.0.0]
    [5.0.0.0.0]
##_LOW_TEXT_IDF_SUM 2 5
    [2.0.0.0.0]
    [5.0.0.0.0]
##_LOW_TEXT_LENS 2 5
    [2.0.0.0.0]
    [5.0.0.0.0]
##_MAX_FREQ 2 6
    [2.0.0.0.1]
    [5.0.0.0.1]
##_NORMAL_TEXT_IDF_SUM 2 12
    [2.1128.32.3.0]
    [5.919.0.3.14]
##_NORMAL_TEXT_LENS 2 8
    [2.0.0.0.6]
    [5.0.0.0.7]
##_TITLE_IDF_SUM 2 5
    [2.0.0.0.0]
    [5.0.0.0.0]
##_TITLE_LENS 2 5
    [2.0.0.0.0]
    [5.0.0.0.0]
#url="localhost/directdoc1 1 4
    [5.0.0.0.0]
#url="localhost/directdoc2 1 4
    [2.0.0.0.0]
a {en} (A) 1 6
    [5.1.1.1.0]
b {en} (b) 1 6
    [5.1.2.1.0]
c {en} (c) 1 6
    [5.1.3.1.0]
d {en} (d) 1 6
    [5.1.4.1.0]
e {en} (E) 1 6
    [5.1.5.1.0]
f {en} (f) 1 6
    [5.1.6.1.0]
g {en} (G) 1 6
    [2.1.1.1.0]
h {en} (h) 1 6
    [2.1.2.1.0]
i {en} (i) 1 6
    [2.1.3.1.0]
j {en} (J) 1 6
    [2.1.4.1.0]
k {en} (k) 1 6
    [2.1.5.1.0]
l {en} (l) 1 6
    [2.1.6.1.0]
x {en} (x) 1 6
    [5.1.7.1.0]
*/

void TAdvancedMergeTest::TestOnlyLemmas() {
    NIndexerCore::TIndexStorageFactory storage;
    storage.InitIndexResources("index", "./", "index");

    {
    NIndexerCore::TInvCreatorConfig invCfg(50);
    NIndexerCore::TInvCreator ic(invCfg);

    ic.AddDoc();
    TPosting pos;
    SetPosting(pos, 0, TWordPosition::FIRST_CHILD);

    const char* lf[][2] = {
        { "a", "aa" }, { "a", "bb" }, { "a", "cc" }, { "a", "dd" },
        { "a", "ee" }, { "a", "ff" }, { "a", "gg" }, { "a", "hh" },

        { "a", "ii" }, { "a", "jj" },
        { "a", "ii" }, { "a", "jj" },

        { "a", "kk" }, { "a", "ll" }, { "a", "mm" },
        { "a", "nn" }, { "a", "oo" }, { "a", "pp" },
        { "a", "nn" }, { "a", "oo" }, { "a", "pp" },
        { "a", "qq" }, { "a", "rr" }, { "a", "ss" },
        { "a", "qq" }, { "a", "rr" }, { "a", "ss" },
        { "a", "tt" }, { "a", "uu" }, { "a", "vv" },

        { "b", "bb" },
        { nullptr, nullptr }
    };

    for (size_t i = 0; lf[i][0]; ++i) {
        TUtf16String lemma = ASCIIToWide(lf[i][0]);
        TUtf16String form = ASCIIToWide(lf[i][1]);
        ic.StoreExternalLemma(lemma.data(), lemma.size(), form.data(), form.size(), 0, LANG_RUS, pos);
        pos = PostingInc(pos);
    }

    ic.CommitDoc(0, 1);
    ic.MakePortion(&storage, true);
    }

    {
        TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
        AddInputPortions(*task, storage.GetNames());
        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index", YNDEX_VERSION_BLK8));
        TAdvancedIndexMerger merger(task);
        merger.Run();
    }
    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("e6bbd23c8cd561704eeacaed2d854bd5", MD5::File("indexkey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("c120e63136afe8814e3aee3e9b336492", MD5::File("indexinv", buf));

    UNIT_ASSERT(unlink("indexkey") == 0);
    UNIT_ASSERT(unlink("indexinv") == 0);
    storage.RemovePortions();
}
/*
$ ../debug/bin/printkeys -cfz indexkey indexinv
a {ru} (aa, bb, cc, dd, ee, ff, gg, hh, ii, jj, nn, oo, pp, qq, rr, ss) 24 47
    [1.0.1.0.0]
    [1.0.2.0.1]
    [1.0.3.0.2]
    [1.0.4.0.3]
    [1.0.5.0.4]
    [1.0.6.0.5]
    [1.0.7.0.6]
    [1.0.8.0.7]
    [1.0.9.0.8]
    [1.0.10.0.9]
    [1.0.11.0.8]
    [1.0.12.0.9]
    [1.0.16.0.10]
    [1.0.17.0.11]
    [1.0.18.0.12]
    [1.0.19.0.10]
    [1.0.20.0.11]
    [1.0.21.0.12]
    [1.0.22.0.13]
    [1.0.23.0.14]
    [1.0.24.0.15]
    [1.0.25.0.13]
    [1.0.26.0.14]
    [1.0.27.0.15]
a {ru} (kk, ll, mm, tt, uu, vv) 6 15
    [1.0.13.0.0]
    [1.0.14.0.1]
    [1.0.15.0.2]
    [1.0.28.0.3]
    [1.0.29.0.4]
    [1.0.30.0.5]
b {ru} (bb) 1 6
    [1.0.31.0.0]
*/

void TAdvancedMergeTest::TestRawKeys() {
    NIndexerCore::TOldIndexFile indexFile(IYndexStorage::FINAL_FORMAT, YNDEX_VERSION_RAW64_HITS);
    indexFile.Open("index0");

    indexFile.StoreNextHit(0x1001001);
    indexFile.StoreNextHit(0x2002002);
    indexFile.StoreNextHit(0x3003003);
    indexFile.FlushNextKey("??????????");

    indexFile.StoreNextHit(0x4004004);
    indexFile.StoreNextHit(0x5005005);
    indexFile.StoreNextHit(0x6006006);
    indexFile.FlushNextKey("one two three");

    indexFile.StoreNextHit(0x7007007);
    indexFile.StoreNextHit(0x8008008);
    indexFile.StoreNextHit(0x9009009);
    indexFile.FlushNextKey("www.yandex.ru/yandsearch?text=blacksabbath");

    indexFile.CloseEx();

    indexFile.Open("index1");

    indexFile.StoreNextHit(0x1001001);
    indexFile.StoreNextHit(0x2002002);
    indexFile.StoreNextHit(0x7007007);
    indexFile.FlushNextKey("one two three");

    indexFile.StoreNextHit(0x2002002);
    indexFile.StoreNextHit(0x4004004);
    indexFile.StoreNextHit(0x6006006);
    indexFile.FlushNextKey("what's up?");

    indexFile.StoreNextHit(0x3003003);
    indexFile.StoreNextHit(0x5005005);
    indexFile.StoreNextHit(0x7007007);
    indexFile.FlushNextKey("www.yandex.ru/yandsearch?text=blacksabbath");

    indexFile.CloseEx();

    {
        // @todo use MergePortions();
        TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());

        task->Inputs.push_back(TAdvancedMergeTask::TMergeInput("index0", IYndexStorage::FINAL_FORMAT));
        task->Inputs.back().Version = YNDEX_VERSION_RAW64_HITS;
        task->Inputs.back().HasSubIndex = false;

        task->Inputs.push_back(TAdvancedMergeTask::TMergeInput("index1", IYndexStorage::FINAL_FORMAT));
        task->Inputs.back().Version = YNDEX_VERSION_RAW64_HITS;
        task->Inputs.back().HasSubIndex = false;

        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index2", YNDEX_VERSION_RAW64_HITS, false));

        TAdvancedIndexMerger merger(task);
        merger.Run();
    }

    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("043b037d49cb5de5ffcadef5431431a6", MD5::File("index2key", buf));
    UNIT_ASSERT_STRINGS_EQUAL("eaf37af02cc53066e4e8652e0ac03634", MD5::File("index2inv", buf));

    UNIT_ASSERT(unlink("index0key") == 0);
    UNIT_ASSERT(unlink("index0inv") == 0);
    UNIT_ASSERT(unlink("index1key") == 0);
    UNIT_ASSERT(unlink("index1inv") == 0);
    UNIT_ASSERT(unlink("index2key") == 0);
    UNIT_ASSERT(unlink("index2inv") == 0);
}
/*
$ ./bin/printkeys -x index2key index2inv
?????????? 3 12
    0000000001001001
    0000000002002002
    0000000003003003
one two three 6 24
    0000000001001001
    0000000002002002
    0000000004004004
    0000000005005005
    0000000006006006
    0000000007007007
what's up? 3 12
    0000000002002002
    0000000004004004
    0000000006006006
www.yandex.ru/yandsearch?text=blacksabbath 6 21
    0000000003003003
    0000000005005005
    0000000007007007
    0000000007007007
    0000000008008008
    0000000009009009
*/

void TAdvancedMergeTest::TestMemoryPortionsImpl(NIndexerCore::TMemoryPortionFactory& memoryPortionFactory) {
    NIndexerCore::TInvCreatorConfig invCfg(50);
    ui32 docID = 0;

    // every MakePortionN() actually creates 2 portions: for lemmas and for attributes
    MakePortion1(memoryPortionFactory, docID, invCfg);
    MakePortion2(memoryPortionFactory, docID, invCfg);

    NIndexerCore::TIndexStorageFactory fileStorageFactory;
    fileStorageFactory.InitIndexResources("index", "./", "index");
    MakePortion3(fileStorageFactory, docID, invCfg);

    { // merge portions
        TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
        AddInputPortions(*task, memoryPortionFactory.GetPortions(), memoryPortionFactory.GetFormat());
        AddInputPortions(*task, fileStorageFactory.GetNames());
        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput("index", YNDEX_VERSION_BLK8));
        TAdvancedIndexMerger merger(task);
        merger.Run();
    }

    fileStorageFactory.RemovePortions();

    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("7b605863c7a639c6a3b3d4ceba372c06", MD5::File("indexkey", buf));
    UNIT_ASSERT_STRINGS_EQUAL("cec004a8fd0d2d37935139fe3db4161b", MD5::File("indexinv", buf));

    UNIT_ASSERT(unlink("indexkey") == 0);
    UNIT_ASSERT(unlink("indexinv") == 0);
}
/*
$ bin/printkeys -cz indexkey indexinv | iconv -f cp1251 -t utf-8
##_DOC_IDF_SUM 7 25
##_DOC_LENS 7 35
##_HEADING_IDF_SUM 7 10
##_HEADING_LENS 7 10
##_LOW_TEXT_IDF_SUM 7 10
##_LOW_TEXT_LENS 7 10
##_MAX_FREQ 7 33
##_NORMAL_TEXT_IDF_SUM 7 25
##_NORMAL_TEXT_LENS 7 35
##_TITLE_IDF_SUM 7 10
##_TITLE_LENS 7 10
#cat="1 3 6
#url="localhost/directdoc00 1 4
#url="localhost/directdoc01 1 4
#url="localhost/directdoc10 1 4
#url="localhost/directdoc11 1 4
#url="localhost/directdoc20 1 4
#url="localhost/directdoc21 1 4
#url="localhost/directdoc22 1 4
00000000001 (000000000001, 00000000001, 0000000001, 000000001, 00000001, 0000001, 000001, 00001, 0001, 001, 001 LR, 001 L+R, 01, 01 LR, 1) 41 69
00000000001 (00000000000000000001, 0000000000000000001, 000000000000000001, 00000000000000001, 0000000000000001, 000000000000001, 00000000000001, 0000000000001, 001 LR+, 001 L+R+, 01 LR+, 01 L+R+, 1 LR, 1 L+R, 1 LR+, 1 L+R+) 109 163
a {en} (a R, a R+) 94 114
b {en} (b L, b L+) 94 113
ближний {ru} (ближайшего, ближайшее, ближайшем, ближайшему, ближайшие, Ближайшие, ближайший, ближайших, ближайшую) 23 42
близкие {ru} (близкие, близким, близкими, близких) 12 25
близкий {ru} (ближайшем, ближайшему, ближайших, ближайшую, близок) 10 25
близкий {ru} (ближайшего, ближайшее, ближайшие, Ближайшие, ближайший, ближе, близкие, Близкий, близким, близкими, близких, близко, Близко, близкой, близком, поближе) 48 70
близко {ru} (ближе, близко, Близко, поближе) 12 30
*/

void TAdvancedMergeTest::TestMemoryPortions() {
    NIndexerCore::TMemoryPortionFactory memoryPortionFactory;
    TestMemoryPortionsImpl(memoryPortionFactory);
}

void TAdvancedMergeTest::TestMemoryPortionsWithVersion() {
    NIndexerCore::TMemoryPortionFactory memoryPortionFactory(IYndexStorage::FINAL_FORMAT, YNDEX_VERSION_BLK8);
    TestMemoryPortionsImpl(memoryPortionFactory);
}

void TAdvancedMergeTest::TestOutputMemoryPortion() {
    const IYndexStorage::FORMAT format = IYndexStorage::FINAL_FORMAT;
    const ui32 version = YNDEX_VERSION_FINAL_DEFAULT;
    NIndexerCore::TMemoryPortionFactory memoryPortionFactory(format, version);
    ui32 docID = 0;

    NIndexerCore::TInvCreatorConfig config(1);
    config.GroupForms = false;
    MakePortion1(memoryPortionFactory, docID, config);
    MakePortion2(memoryPortionFactory, docID, config);
    MakePortion3(memoryPortionFactory, docID, config);

    // merge portions
    TVector<TPortionBuffers> portionBuffers;
    TVector<ui32> remap;
    const TVector<const NIndexerCore::TMemoryPortion*> portions = memoryPortionFactory.GetPortions();
    for (size_t i = 0; i < portions.size(); ++i) {
        const NIndexerCore::TMemoryPortion* p = portions[i];
        const TBuffer& keybuf = p->GetKeyBuffer();
        const TBuffer& invbuf = p->GetInvBuffer();
        portionBuffers.push_back(TPortionBuffers(keybuf.Data(), keybuf.Size(), invbuf.Data(), invbuf.Size()));
    }
    NIndexerCore::TMemoryPortion outputPortion(format, version);
    MergeMemoryPortions(&portionBuffers[0], portionBuffers.size(), format, nullptr, false, outputPortion);

    const char keyname[] = "portionkey";
    const char invname[] = "portioninv";
{
    TFile keyfile(keyname, CreateAlways | WrOnly);
    keyfile.Write(outputPortion.GetKeyBuffer().Data(), outputPortion.GetKeyBuffer().Size());
    TFile invfile(invname, CreateAlways | WrOnly);
    invfile.Write(outputPortion.GetInvBuffer().Data(), outputPortion.GetInvBuffer().Size());
}
    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("d84245c1bfcdc8d42209972ac5e7f1b5", MD5::File(keyname, buf));
    UNIT_ASSERT_STRINGS_EQUAL("1b19516bccff9b94540fae81636b2dab", MD5::File(invname, buf));

    UNIT_ASSERT(unlink(keyname) == 0);
    UNIT_ASSERT(unlink(invname) == 0);
}
/*
$ bin/printkeys -cz portionkey portioninv | iconv -f cp1251 -t utf-8
##_DOC_IDF_SUM 7 25
##_DOC_LENS 7 35
##_HEADING_IDF_SUM 7 10
##_HEADING_LENS 7 10
##_LOW_TEXT_IDF_SUM 7 10
##_LOW_TEXT_LENS 7 10
##_MAX_FREQ 7 33
##_NORMAL_TEXT_IDF_SUM 7 25
##_NORMAL_TEXT_LENS 7 35
##_TITLE_IDF_SUM 7 10
##_TITLE_LENS 7 10
#cat="1 3 6
#url="localhost/directdoc00 1 4
#url="localhost/directdoc01 1 4
#url="localhost/directdoc10 1 4
#url="localhost/directdoc11 1 4
#url="localhost/directdoc20 1 4
#url="localhost/directdoc21 1 4
#url="localhost/directdoc22 1 4
00000000001 (00000000001) 3 8
00000000001 (1) 2 7
00000000001 (01) 2 7
00000000001 (001) 2 7
00000000001 (0001) 2 7
00000000001 (00001) 3 8
00000000001 (000001) 3 8
00000000001 (0000001) 3 8
00000000001 (00000001) 3 8
00000000001 (000000001) 3 8
00000000001 (0000000001) 3 8
00000000001 (000000000001) 3 8
00000000001 (1 LR) 4 9
00000000001 (1 L+R) 4 9
00000000001 (1 LR+) 23 41
00000000001 (1 L+R+) 15 33
00000000001 (01 LR) 3 8
00000000001 (01 LR+) 10 29
00000000001 (01 L+R+) 12 32
00000000001 (001 LR) 3 8
00000000001 (001 L+R) 3 8
00000000001 (001 LR+) 7 18
00000000001 (001 L+R+) 10 29
00000000001 (0000000000001) 3 8
00000000001 (00000000000001) 3 8
00000000001 (000000000000001) 3 8
00000000001 (0000000000000001) 3 8
00000000001 (00000000000000001) 3 8
00000000001 (000000000000000001) 3 8
00000000001 (0000000000000000001) 3 8
00000000001 (00000000000000000001) 3 8
a {en} (a R) 50 72
a {en} (a R+) 44 62
b {en} (b L) 17 39
b {en} (b L+) 77 94
ближний {ru} (ближайшее) 3 8
ближний {ru} (ближайшем) 2 7
ближний {ru} (ближайшие) 3 8
ближний {ru} (ближайший) 3 8
ближний {ru} (ближайших) 2 7
ближний {ru} (ближайшую) 2 7
ближний {ru} (ближайшего) 3 8
ближний {ru} (ближайшему) 2 7
ближний {ru} (Ближайшие) 3 8
близкие {ru} (близкие) 3 8
близкие {ru} (близким) 3 8
близкие {ru} (близких) 3 8
близкие {ru} (близкими) 3 8
близкий {ru} (близко) 3 8
близкий {ru} (близкие) 3 8
близкий {ru} (близким) 3 8
близкий {ru} (близких) 3 8
близкий {ru} (Близкий) 3 8
близкий {ru} (ближе) 3 8
близкий {ru} (близок) 2 7
близкий {ru} (Близко) 3 8
близкий {ru} (близкой) 3 8
близкий {ru} (близком) 3 8
близкий {ru} (близкими) 3 8
близкий {ru} (ближайшее) 3 8
близкий {ru} (ближайшем) 2 7
близкий {ru} (ближайшие) 3 8
близкий {ru} (ближайший) 3 8
близкий {ru} (ближайших) 2 7
близкий {ru} (ближайшую) 2 7
близкий {ru} (поближе) 3 8
близкий {ru} (ближайшего) 3 8
близкий {ru} (ближайшему) 2 7
близкий {ru} (Ближайшие) 3 8
близко {ru} (близко) 3 8
близко {ru} (Близко) 3 8
близко {ru} (ближе) 3 8
близко {ru} (поближе) 3 8
*/

void TAdvancedMergeTest::TestOutputIntersectedMemoryPortions() {
    const IYndexStorage::FORMAT format = IYndexStorage::FINAL_FORMAT;
    const ui32 version = YNDEX_VERSION_FINAL_DEFAULT;
    NIndexerCore::TMemoryPortionFactory memoryPortionFactory(format, version);
    ui32 docID = 0;

    NIndexerCore::TInvCreatorConfig config(1);
    config.GroupForms = false;
    MakePortion1(memoryPortionFactory, docID, config);
    docID = 0;
    MakePortion1(memoryPortionFactory, docID, config);
    docID = 1;
    MakePortion2(memoryPortionFactory, docID, config);

    // merge portions
    TVector<TPortionBuffers> portionBuffers;
    TVector<ui32> remap;
    const TVector<const NIndexerCore::TMemoryPortion*> portions = memoryPortionFactory.GetPortions();
    for (size_t i = 0; i < portions.size(); ++i) {
        const NIndexerCore::TMemoryPortion* p = portions[i];
        const TBuffer& keybuf = p->GetKeyBuffer();
        const TBuffer& invbuf = p->GetInvBuffer();
        portionBuffers.push_back(TPortionBuffers(keybuf.Data(), keybuf.Size(), invbuf.Data(), invbuf.Size()));
    }
    NIndexerCore::TMemoryPortion outputPortion(format, version);
    MergeMemoryPortions(&portionBuffers[0], portionBuffers.size(), format, nullptr,
        /*intersectedRanges=*/true, outputPortion);

    const char keyname[] = "portionkey";
    const char invname[] = "portioninv";
{
    TFile keyfile(keyname, CreateAlways | WrOnly);
    keyfile.Write(outputPortion.GetKeyBuffer().Data(), outputPortion.GetKeyBuffer().Size());
    TFile invfile(invname, CreateAlways | WrOnly);
    invfile.Write(outputPortion.GetInvBuffer().Data(), outputPortion.GetInvBuffer().Size());
}
    char buf[50];
    UNIT_ASSERT_STRINGS_EQUAL("70c525b0f9029dbc69b20e24045b1a27", MD5::File(keyname, buf));
    UNIT_ASSERT_STRINGS_EQUAL("1f9604751bdd5467f3f5c9f94fbc5ec6", MD5::File(invname, buf));

    UNIT_ASSERT(unlink(keyname) == 0);
    UNIT_ASSERT(unlink(invname) == 0);
}
/*
$ bin/printkeys -cz portionkey portioninv
##_DOC_IDF_SUM 6 23
##_DOC_LENS 6 19
##_HEADING_IDF_SUM 6 9
##_HEADING_LENS 6 9
##_LOW_TEXT_IDF_SUM 6 9
##_LOW_TEXT_LENS 6 9
##_MAX_FREQ 6 19
##_NORMAL_TEXT_IDF_SUM 6 23
##_NORMAL_TEXT_LENS 6 19
##_TITLE_IDF_SUM 6 9
##_TITLE_LENS 6 9
#url="localhost/directdoc00 2 5
#url="localhost/directdoc01 2 5
#url="localhost/directdoc10 1 4
#url="localhost/directdoc11 1 4
00000000001 (00000000001) 3 8
00000000001 (1) 2 7
00000000001 (01) 2 7
00000000001 (001) 2 7
00000000001 (0001) 2 7
00000000001 (00001) 3 8
00000000001 (000001) 3 8
00000000001 (0000001) 3 8
00000000001 (00000001) 3 8
00000000001 (000000001) 3 8
00000000001 (0000000001) 3 8
00000000001 (000000000001) 3 8
00000000001 (0000000000001) 3 8
00000000001 (00000000000001) 3 8
00000000001 (000000000000001) 3 8
00000000001 (0000000000000001) 3 8
00000000001 (00000000000000001) 3 8
00000000001 (000000000000000001) 3 8
00000000001 (0000000000000000001) 3 8
00000000001 (00000000000000000001) 3 8
ближний {ru} (ближайшее) 6 11
ближний {ru} (ближайшем) 4 9
ближний {ru} (ближайшие) 6 11
ближний {ru} (ближайший) 6 11
ближний {ru} (ближайших) 4 9
ближний {ru} (ближайшую) 4 9
ближний {ru} (ближайшего) 6 11
ближний {ru} (ближайшему) 4 9
ближний {ru} (Ближайшие) 6 11
близкие {ru} (близкие) 6 11
близкие {ru} (близким) 6 11
близкие {ru} (близких) 6 11
близкие {ru} (близкими) 6 11
близкий {ru} (близко) 6 11
близкий {ru} (близкие) 6 11
близкий {ru} (близким) 6 11
близкий {ru} (близких) 6 11
близкий {ru} (Близкий) 6 11
близкий {ru} (ближе) 6 11
близкий {ru} (близок) 4 9
близкий {ru} (Близко) 6 11
близкий {ru} (близкой) 6 11
близкий {ru} (близком) 6 11
близкий {ru} (близкими) 6 11
близкий {ru} (ближайшее) 6 11
близкий {ru} (ближайшем) 4 9
близкий {ru} (ближайшие) 6 11
близкий {ru} (ближайший) 6 11
близкий {ru} (ближайших) 4 9
близкий {ru} (ближайшую) 4 9
близкий {ru} (поближе) 6 11
близкий {ru} (ближайшего) 6 11
близкий {ru} (ближайшему) 4 9
близкий {ru} (Ближайшие) 6 11
близко {ru} (близко) 6 11
близко {ru} (Близко) 6 11
близко {ru} (ближе) 6 11
близко {ru} (поближе) 6 11
*/
