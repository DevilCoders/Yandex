#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/memory/blob.h>
#include <util/system/tempfile.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/json/json_reader.h>

#include <library/cpp/deprecated/omni/write.h>
#include <library/cpp/deprecated/omni/read.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/digest/md5/md5.h>

struct TRndGen {
    TRndGen()
        : State(0)
    {
    }
    unsigned int Get() {
        State = State * 1664525u + 1013904223u;
        return State;
    }

private:
    unsigned int State;
};

TVector<TString> GenerateUrls() {
    TRndGen gen;
    TVector<TString> lines;
    for (size_t i = 0; i < 100000; ++i) {
        size_t size = gen.Get() % 32;
        TString res = "www.yandex.ru/yandsearch?text=";
        for (size_t j = 0; j < size; ++j) {
            res += "qwer"[gen.Get() % 4];
        }
        lines.push_back(res);
    }
    return lines;
}

using namespace NOmni;

void ArchiveCallbackLoop(IStackIndexer* indexer, const TVector<TString>& urls) {
    indexer->StartMap(0);
    size_t n = urls.size();
    for (size_t i = 0; i < n; ++i) {
        indexer->StartMap(i);
        indexer->WriteData(0, "hello world!");
        indexer->WriteData(1, urls[i]);
        indexer->EndMap(); //doc
    }
    indexer->EndMap(); //main docs
    indexer->StartMap(1);
    indexer->WriteData(0, "14.07.1790");
    indexer->WriteData(1, "/place/home/snow");
    indexer->EndMap(); // meta info
}

void ArchiveMapLoop(IStackIndexer& indexer, const TVector<std::pair<size_t, TString>>& docs) {
    indexer.StartMap(0);
    size_t n = docs.size();
    for (size_t i = 0; i < n; ++i) {
        size_t docId = docs[i].first;
        indexer.StartMap(docId);
        indexer.WriteData(0, "hello world!");
        indexer.WriteData(1, docs[i].second);
        indexer.EndMap(); //doc
    }
    indexer.EndMap(); //main docs
    indexer.StartMap(1);
    indexer.WriteData(0, "14.07.1790");
    indexer.WriteData(1, "/place/home/snow");
    indexer.EndMap(); // meta info
}

//#define DO_MY_UNITTEST_TEST 1

Y_UNIT_TEST_SUITE(TestOmnidex) {
    Y_UNIT_TEST(TestOmnidexReadWrite) {
        srand(1);

#ifdef DO_MY_UNITTEST_TEST
        TString indexPath = "/place/home/snow/index.db";
        TString dumpPath = "/place/home/snow/index.dump";
        TFixedBufferFileOutput dbgLog("/place/home/snow/ut.log");
#else
        TString indexPath = "index.db";
        TString dumpPath = "index.dump";
        TTempFile tmpi(indexPath);
        TTempFile tmpd(dumpPath);
#endif

        {
            TString schemePath = ArcadiaSourceRoot() + "/library/cpp/deprecated/omni/ut/scheme.js";
            TVector<TString> urls = GenerateUrls();
            TStatCollector collector((TJsonFileScheme(schemePath)));
            ArchiveCallbackLoop(&collector, urls);
            collector.Finish();
            TVector<TBlob> dynamicTables = collector.GetComputedCodecTables();
            TFileArchiver indexer(TJsonFileScheme(schemePath), indexPath, &dynamicTables);
            ArchiveCallbackLoop(&indexer, urls);
            indexer.Finish();
        }
        TString md5str = MD5::File(indexPath);
        UNIT_ASSERT_EQUAL(md5str, "aa13126be2362d0dccd4af3656ec0078");
        {
            TFixedBufferFileOutput fdump(dumpPath);
            TOmniReader reader(indexPath);
            TOmniIterator iter = reader.Root();
            iter.DbgPrint(fdump);
            fdump.Finish();
        }
        md5str = MD5::File(dumpPath);
        UNIT_ASSERT_EQUAL(md5str, "49c3f9d46563041b6664debae419bead");
    }

    Y_UNIT_TEST(TestOmnidexMapAndZlib) {
        srand(1);

#ifdef DO_MY_UNITTEST_TEST
        TString indexPath = "/place/home/snow/index2.db";
        TFixedBufferFileOutput dbgLog("/place/home/snow/ut.log");
#else
        TString indexPath = "index.db";
        TTempFile tmpi(indexPath);
#endif

        {
            size_t goodIds[] = {4, 7, 19, 20, 70, 88, 100, 314, 1000, 5000};
            TString schemePath = ArcadiaSourceRoot() + "/library/cpp/deprecated/omni/ut/map_scheme.js";
            TVector<TString> urls = GenerateUrls();
            TVector<std::pair<size_t, TString>> docs;
            for (auto& goodId : goodIds) {
                docs.push_back(std::make_pair(goodId, urls[goodId]));
            }
            TStatCollector collector((TJsonFileScheme(schemePath)));
            ArchiveMapLoop(collector, docs);
            collector.Finish();
            TVector<TBlob> dynamicTables = collector.GetComputedCodecTables();
            TFileArchiver indexer(TJsonFileScheme(schemePath), indexPath, &dynamicTables);
            ArchiveMapLoop(indexer, docs);
            indexer.Finish();
        }
        {
            TVector<char> dataBuf;
            TOmniReader reader(indexPath);
            TOmniIterator root = reader.Root();
            TOmniIterator docs = root.GetByKey(0);
            Y_UNUSED(docs);
            UNIT_ASSERT(docs.HasKey(4)); //first
            UNIT_ASSERT(docs.HasKey(1000));
            UNIT_ASSERT(!docs.HasKey(0)); //before first
            UNIT_ASSERT(!docs.HasKey(16));
            UNIT_ASSERT(docs.HasKey(5000));    //last
            UNIT_ASSERT(!docs.HasKey(100000)); //after last
            docs.GetByKey(7).GetByPos(0).GetData(&dataBuf);
            UNIT_ASSERT_EQUAL(TStringBuf(dataBuf.data(), dataBuf.size()), "hello world!");
            root.GetByKey(1).GetByPos(0).GetData(&dataBuf);
            UNIT_ASSERT_EQUAL(TStringBuf(dataBuf.data(), dataBuf.size()), "14.07.1790");
            root.GetByKey(1).GetByPos(1).GetData(&dataBuf);
            UNIT_ASSERT_EQUAL(TStringBuf(dataBuf.data(), dataBuf.size()), "/place/home/snow");
        }
    }
}
