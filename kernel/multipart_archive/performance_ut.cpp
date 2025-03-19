#include <kernel/multipart_archive/hash/hash.h>
#include <kernel/multipart_archive/multipart.h>
#include <kernel/multipart_archive/queue/queue_storage.h>
#include <kernel/multipart_archive/config/config.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/thread/pool.h>
#include <util/random/shuffle.h>

#include "archive_test_base.h"
#include "performance_test.h"


using namespace NRTYArchive;
using namespace NRTYArchiveTest;

class TPerformanceSuiteBase: public TMultipartArchiveTestBase {
public:
    ELogPriority GetLogLevel() const override {
        return TLOG_INFO;
    }
};

Y_UNIT_TEST_SUITE_IMPL(MultipartPerformance, TPerformanceSuiteBase) {
    class THashStorage : public IStorageWrapper<TString> {
        using TTestData = TTestData<TString>;

    public:
        THashStorage(TMultipartHash& hash)
            : Hash(hash)
        {}

        void Prepare(const TVector<TTestData>& testData) override {
            for (auto&& data : testData) {
                Hash.Insert(data.Key, data.Value);
            }
        }

        TBlob DoRead(const TTestData& data) const override {
            return Hash.Find(data.Key);
        }

        void DoWrite(const TTestData& data) override {
            Hash.Insert(data.Key, data.Value);
        }

    private:
        TMultipartHash& Hash;
    };

    Y_UNIT_TEST(TestHashReadWrite) {
        TMultipartHashImpl::Remove("test");
        TMultipartConfig config;
        TMultipartHash hash("test", config.CreateReadContext(), TMaybe<ui32>());
        THashStorage hashWrapper(hash);
        TTestStat results;
        TPerformanceStat<TString>(12000, 3, 3, hashWrapper).Run<TStrKeyGenerator>(TDuration::Seconds(30), results);
        results.Save("TestHashReadWrite");
    }

    class TArchiveStorage : public IStorageWrapper<ui32> {
        using TTestData = TTestData<ui32>;

    public:
        TArchiveStorage(TEngine::TPtr archive)
            : Archive(archive)
        {}

        void Prepare(const TVector<TTestData>& testData) override {
            for (auto&& data : testData) {
                Archive->PutDocument(data.Value, data.Key);
            }
            Archive->Flush();
            INFO_LOG << "init_parts_count=" << Archive->GetPartsCount() << Endl;
        }

        TBlob DoRead(const TTestData& data) const override {
            return Archive->ReadDocument(data.Key);
        }

        void DoWrite(const TTestData& data) override {
            Archive->PutDocument(data.Value, data.Key);
        }

        void Optimize() override {
            TOptimizationOptions opt;
            Archive->OptimizeParts(opt);
        }
    private:
        TEngine::TPtr Archive;
    };

    Y_UNIT_TEST(TestArchiveReadWrite) {
        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;
        auto archive = TEngine::Create("test", config, 0);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 3, 3, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(30), results);
        INFO_LOG << "read_rps:" << results.ReadRps << ";write_rps:" << results.WriteRps << Endl;
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    Y_UNIT_TEST(TestArchiveReadWriteFast) {
        TMultipartConfig config;
        config.WritableThreadsCount = 3;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 3, 3, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(30), results);
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    Y_UNIT_TEST(TestArchiveReadWriteMultipleParts) {
        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;
        config.PartSizeLimit = 1000000;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 3, 3, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(30), results);
        INFO_LOG << "parts_count=" << archive->GetPartsCount() << Endl;
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    // Read
    Y_UNIT_TEST(TestArchiveRead) {
        TMultipartConfig config;
        config.PartSizeLimit = 100000;
        config.ReadContextDataAccessType = IDataAccessor::MEMORY_FROM_FILE;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 0, 6, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(30), results);
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    // Write
    Y_UNIT_TEST(TestArchiveWrite) {
        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 6, 0, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(30), results);
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }


    void TestArchiveWriteCheckImpl(ui32 writablePartsCount, NUnitTest::TTestContext& ut_context) {
        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        config.WritableThreadsCount = writablePartsCount;

        ui32 docsCount = 12000;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);

        {
            TTestDataGenerator<TUI32KeyGenerator>  generator(0);
            TVector<TTestData<ui32>> testData;
            generator.Generate(docsCount, testData);
            INFO_LOG << "Generate data OK (" << testData.size() << ")" << Endl;
            wrapper.Prepare(testData);
        }

        TVector<TTestData<ui32>> newTestData;
        {
            TTestDataGenerator<TUI32KeyGenerator>  generator(0);
            generator.Generate(docsCount, newTestData);
            INFO_LOG << "Generate new data OK (" << newTestData.size() << ")" << Endl;
        }

        TTestStat results;
        TPerformanceStat<ui32>(docsCount, 6, 0, wrapper).RunExternal(TDuration::Seconds(30), results, newTestData);

        TSet<ui32> sizes;
        for (ui32 index = 0; index < newTestData.size(); ++index) {
            TTestData<ui32> data = newTestData[index];
            TBlob val = wrapper.DoRead(data);
            CHECK_WITH_LOG(!val.Empty());
            sizes.insert(data.Value.Size());

            if (val.Size() != data.Value.Size()) {
                INFO_LOG << "key=" << data.Key << ";real=" << val.Size() << ";expected=" << data.Value.Size() << Endl;
            }
            CHECK_WITH_LOG(val.Size() == data.Value.Size());
        }

        UNIT_ASSERT(sizes.size() > 1);
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    Y_UNIT_TEST(TestArchiveWriteCheck) {
        TestArchiveWriteCheckImpl(1, ut_context);
    }

    Y_UNIT_TEST(TestArchiveWrite3PartsCheck) {
        TestArchiveWriteCheckImpl(3, ut_context);
    }

    Y_UNIT_TEST(TestArchiveWriteFast) {
        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        config.WritableThreadsCount = 3;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 6, 0, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(30), results);
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    Y_UNIT_TEST(TestArchiveWriteMultipleParts) {
        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        config.PartSizeLimit = 1000000;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 6, 0, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(10), results, false, 1000);
        INFO_LOG << "parts_count=" << archive->GetPartsCount() << Endl;
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    // Optimize
    Y_UNIT_TEST(TestArchiveWriteAndOptimize) {
        TMultipartConfig config;
        config.PartSizeLimit = 100000;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 6, 0, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(10), results, true);
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    Y_UNIT_TEST(TestArchiveConcurrentOptimize) {
        TMultipartConfig config;
        config.PartSizeLimit = 100000;
        auto archive = TEngine::Create("test", config, 12000);
        TArchiveStorage wrapper(archive);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 3, 3, wrapper).Run<TUI32KeyGenerator>(TDuration::Seconds(10), results, true);
        UNIT_ADD_METRIC("read_rps", results.ReadRps);
        UNIT_ADD_METRIC("write_rps", results.WriteRps);
    }

    class TQueueStorage : public IStorageWrapper<ui32> {
        using TTestData = TTestData<ui32>;

    public:
        TQueueStorage(TStorage& archive)
            : Storage(archive)
        {}

        void Prepare(const TVector<TTestData>& testData) override {
            for (auto&& data : testData) {
                Storage.Put(data.Value);
            }
        }

        TBlob DoRead(const TTestData& data) const override {
            auto doc = Storage.Get(false);
            if (doc) {
                TString dataReaded((char*)doc->GetBlob().Data(), doc->GetBlob().Size());
                CHECK_WITH_LOG(dataReaded.size() > 0);
                CHECK_WITH_LOG(dataReaded.size() == TSizeByCharBlobGenerator::SizeBySymb(dataReaded[0]));
                doc->Ack();
                AtomicIncrement(DocsRead);
            }
            return data.Value;
        }

        void DoWrite(const TTestData& data) override {
            Storage.Put(data.Value);
        }

        ui32 GetReadCount() const {
            return AtomicGet(DocsRead);
        }

    private:
        TStorage& Storage;
        mutable TAtomic DocsRead = 0;
    };

    Y_UNIT_TEST(TestQueueReadWrite) {
        TMultipartArchive::Remove("test");
        TMultipartConfig config;
        TStorage storage("test", TStorage::TLimits(), TMultipartConfig());
        TQueueStorage wrapper(storage);
        TTestStat results;
        TPerformanceStat<ui32>(12000, 3, 3, wrapper).Run<TUI32KeyGenerator, TSizeByCharBlobGenerator>(TDuration::Seconds(30), results);

        ui32 docsCount = storage.GetDocsCount();
        ui32 queueSize = 0;
        auto doc = storage.Get(false);
        while(doc) {
            doc->Ack();
            doc = storage.Get(false);
            queueSize++;
        }

        INFO_LOG << "real_read=" << wrapper.GetReadCount() << ";queue_size=" << docsCount <<  Endl;
        UNIT_ASSERT_VALUES_EQUAL(queueSize, docsCount);
    }

    void TestIterPerformanceImpl(bool /*useBlocks*/) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(2000000, data, 2000);
        auto archive  = TArchiveConstructor::ConstructCompressedMem(data, 2);
        TFsPath path = archive->GetPath();
        archive.Drop();
        archive  = TArchiveConstructor::GetCompressedMem(path, 10000000);

        TOptimizationOptions opt;
    //    opt.SetUseBlocks(useBlocks);

        TInstant start = TInstant::Now();
        archive->OptimizeParts(opt);
        TDuration delta = TInstant::Now() - start;
        INFO_LOG << "opt_time_ms=" << delta.MilliSeconds() << Endl;
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(true), data.size());
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), data.size());
    }

    Y_UNIT_TEST(TestBlocksIterPerformance) {
        TestIterPerformanceImpl(true);
    }

    Y_UNIT_TEST(TestIterPerformance) {
        TestIterPerformanceImpl(false);
    }

    Y_UNIT_TEST(TestArchiveRemovingTimes) {
        TFsPath arcPath("test_archive");

        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;

        {
            auto archive = TEngine::Create(arcPath, config, 0);

            for (ui32 docId = 0; docId < 1000000; ++docId) {
                archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
            }
        }

        TFsPath arcCopy(arcPath.GetPath() + "_copy");

        TDuration deltaEndToStart;
        TDuration deltaStartToEnd;
        TDuration deltaRandom;

        TFsPath(GetFatPath(arcPath)).CopyTo(GetFatPath(arcCopy), true);
        TFsPath(GetPartPath(arcPath, 0)).CopyTo(GetPartPath(arcCopy, 0), true);
        TFsPath(GetPartHeaderPath(arcPath, 0)).CopyTo(GetPartHeaderPath(arcCopy, 0), true);
        TFsPath(GetPartMetaPath(arcPath, 0)).CopyTo(GetPartMetaPath(arcCopy, 0), true);

        config.ReadContextDataAccessType = IDataAccessor::MEMORY_FROM_FILE;
        {
            auto archive = TEngine::Create(arcPath, config);

            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 1000000);
            TInstant start = Now();
            for (ui32 docId = 1000000; docId > 0; --docId) {
                archive->RemoveDocument(docId - 1);
            }

            deltaEndToStart = (Now() - start);
            INFO_LOG << "End to start: " << deltaEndToStart.MilliSeconds() << Endl;
        }

        TFsPath(GetFatPath(arcCopy)).CopyTo(GetFatPath(arcPath), true);
        TFsPath(GetPartPath(arcCopy, 0)).CopyTo(GetPartPath(arcPath, 0), true);
        TFsPath(GetPartHeaderPath(arcCopy, 0)).CopyTo(GetPartHeaderPath(arcPath, 0), true);
        TFsPath(GetPartMetaPath(arcCopy, 0)).CopyTo(GetPartMetaPath(arcPath, 0), true);

        {
            auto archive = TEngine::Create(arcPath, config);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 1000000);

            TInstant start = Now();
            for (ui32 docId = 0; docId < 1000000; ++docId) {
                archive->RemoveDocument(docId);
            }

            deltaStartToEnd = (Now() - start);
            INFO_LOG << "Start to end: " << deltaStartToEnd.MilliSeconds() << Endl;
        }

        TFsPath(GetFatPath(arcCopy)).RenameTo(GetFatPath(arcPath));
        TFsPath(GetPartPath(arcCopy, 0)).RenameTo(GetPartPath(arcPath, 0));
        TFsPath(GetPartHeaderPath(arcCopy, 0)).RenameTo(GetPartHeaderPath(arcPath, 0));
        TFsPath(GetPartMetaPath(arcCopy, 0)).RenameTo(GetPartMetaPath(arcPath, 0));

        {
            auto archive = TEngine::Create(arcPath, config);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 1000000);

            TVector<ui32> docIds;
            for (ui32 docId = 0; docId < 1000000; ++docId) {
                docIds.push_back(docId);
            }

            Shuffle(docIds.begin(), docIds.end());

            TInstant start = Now();
            for (ui32 i = 0; i < 1000000; ++i) {
                archive->RemoveDocument(docIds[i]);
            }

            deltaRandom = (Now() - start);
            INFO_LOG << "Random: " << deltaRandom.MilliSeconds() << Endl;
        }

        UNIT_ASSERT(Abs<i64>(deltaRandom.MilliSeconds() - deltaEndToStart.MilliSeconds()) < 1000);
        UNIT_ASSERT(Abs<i64>(deltaStartToEnd.MilliSeconds() - deltaEndToStart.MilliSeconds()) < 1000);
    }
}
