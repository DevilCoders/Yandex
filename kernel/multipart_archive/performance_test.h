#pragma once

#include "test_utils.h"
#include <library/cpp/json/json_writer.h>

#include <util/stream/file.h>

namespace NRTYArchiveTest {

    class TTestStat {
    public:
        void Save(const TString testName) const {
            NJson::TJsonValue json;
            json["read_rps?aggr=max"] = ReadRps;
            json["write_rps?aggr=max"] = WriteRps;

            TFileOutput out(testName + ".info");
            NJson::WriteJson(&out, &json);
        }

    public:
        ui32 ReadRps = 0;
        ui32 WriteRps = 0;
    };

    template<class TKey>
    class IStorageWrapper {
    public:
        virtual void Prepare(const TVector<TTestData<TKey>>& data) = 0;
        virtual TBlob DoRead(const TTestData<TKey>& data) const = 0;
        virtual void DoWrite(const TTestData<TKey>& data) = 0;
        virtual void Optimize() {};
        virtual ~IStorageWrapper() {}
    };

    template<class TKey>
    class TPerformanceStat {
        using TStat = TPerformanceStat<TKey>;
        using TTestData = TTestData<TKey>;

    private:
        TAtomic ReadingCount;
        TAtomic WritingCount;
        TAtomic OptimizingCount;;

        ui32 WritersCount = 0;
        ui32 ReadersCount = 0;
        ui32 DocsCount = 0;
        ui32 DocsPerWriter = 0;
        ui32 DocsPerReader = 0;

        TThreadPool Writers;
        TThreadPool Readers;

        IStorageWrapper<TKey>& Storage;
        TVector<TTestData> TestData;

    public:
        TPerformanceStat(ui32 docsCount, ui32 writersCount, ui32 readersCount, IStorageWrapper<TKey>& storage)
            : ReadingCount(0)
            , WritingCount(0)
            , OptimizingCount(0)
            , WritersCount(writersCount)
            , ReadersCount(readersCount)
            , DocsCount(docsCount)
            , DocsPerWriter(writersCount ? docsCount / writersCount : 0)
            , DocsPerReader(readersCount ? docsCount / readersCount : 0)
            , Storage(storage)
        {
            if (writersCount) {
                CHECK_WITH_LOG(DocsPerWriter * writersCount == docsCount);
            }
            if (readersCount) {
                CHECK_WITH_LOG(DocsPerReader * readersCount == docsCount);
            }
        }

        void RegisterRead() {
            AtomicIncrement(ReadingCount);
        }

        void RegisterWrite() {
            AtomicIncrement(WritingCount);
        }

        void RegisterOptimize() {
            AtomicIncrement(OptimizingCount);
        }

        void RunExternal(const TDuration& testTime, TTestStat& results, const TVector<TTestData>& externalTestData, bool runOptimizer = false) {
            TestData = externalTestData;
            CHECK_WITH_LOG(DocsCount == TestData.size());
            TInstant deadline = TInstant::Now() + testTime;

            INFO_LOG << "Start Writers (" << WritersCount << ")" << Endl;
            Writers.Start(WritersCount);

            for (ui32 i = 0; i < WritersCount; ++i) {
                CHECK_WITH_LOG(Writers.AddAndOwn(MakeHolder<TWriterTask>(i, DocsPerWriter, Storage, *this, deadline)));
            }

            INFO_LOG << "Start Readers (" << ReadersCount << ")" << Endl;
            Readers.Start(ReadersCount);
            for (ui32 i = 0; i < ReadersCount; ++i) {
                CHECK_WITH_LOG(Readers.AddAndOwn(MakeHolder<TReaderTask>(i, DocsPerReader, Storage, *this, deadline)));
            }

            if (runOptimizer) {
                TThreadPool optimizer;
                optimizer.Start(1);
                CHECK_WITH_LOG(optimizer.AddAndOwn(MakeHolder<TOptimizerTask>(Storage, *this, deadline)));
            }

            Writers.Stop();
            INFO_LOG << "Stop Writers" << Endl;
            Readers.Stop();
            INFO_LOG << "Stop Readers" << Endl;


            results.ReadRps = AtomicGet(ReadingCount) / testTime.Seconds();
            results.WriteRps = AtomicGet(WritingCount) / testTime.Seconds();
            INFO_LOG << "read_rps=" << results.ReadRps
                << ";write_rps=" << results.WriteRps
                << ";optimizer_tasks=" << AtomicGet(OptimizingCount)
                << Endl;
        }

        template<class TKeyGenerator, class TBlobGenerator = TDefaultBlobGenerator>
        void Run(const TDuration& testTime, TTestStat& results, bool runOptimizer = false, ui32 docSize = 0) {
            INFO_LOG << "Generate data" << Endl;
            TTestDataGenerator<TKeyGenerator, TBlobGenerator>  generator(docSize);
            TVector<TTestData> testData;
            generator.Generate(DocsCount, testData);
            INFO_LOG << "Generate data OK ("  << testData.size() << ")" << Endl;

            INFO_LOG << "Prepare data storage" << Endl;

            Storage.Prepare(testData);

            RunExternal(testTime, results, testData, runOptimizer);
        }

    private:
        class TOptimizerTask: public IObjectInQueue {
        public:
            TOptimizerTask(IStorageWrapper<TKey>& storage, TStat& stat, TInstant deadline)
                : Stat(stat)
                , Storage(storage)
                , Deadline(deadline)
            {}

            virtual void Process(void*) override {
                while(true) {
                    Sleep(TDuration::MilliSeconds(200));
                    TInstant start = TInstant::Now();
                    Storage.Optimize();
                    Stat.RegisterOptimize();
                    if (start > Deadline) {
                        return;
                    }
                }
            }

        protected:
            TStat& Stat;
            IStorageWrapper<TKey>& Storage;
            TInstant Deadline;
        };

        class TTaskWithDeadline: public IObjectInQueue {
        public:
            TTaskWithDeadline(ui32 id, ui32 docsCount, IStorageWrapper<TKey>& storage, TStat& stat, TInstant deadline)
                : Stat(stat)
                , Id(id)
                , DocsCount(docsCount)
                , Storage(storage)
                , Deadline(deadline)
            {}

            virtual void Process(void*) override {
                while(true) {
                    for (ui32 i = 0; i < DocsCount; ++i) {
                        const TTestData& data = Stat.GetData(Id, i, DocsCount);
                        TInstant start = TInstant::Now();
                        ProcessData(data);
                        TDuration time = TInstant::Now() - start;
                        UpdateTimes(time);

                        if (start > Deadline) {
                            return;
                        }
                    }
                }
            }

            virtual void ProcessData(const TTestData& data) = 0;
            virtual void UpdateTimes(const TDuration& time) = 0;

        protected:
            TStat& Stat;
            ui32 Id = 0;
            ui32 DocsCount = 0;
            IStorageWrapper<TKey>& Storage;
            TInstant Deadline;
        };

        class TWriterTask : public TTaskWithDeadline {
            using TBase = TTaskWithDeadline;
        public:
            TWriterTask(ui32 id, ui32 docsCount, IStorageWrapper<TKey>& storage, TStat& stat, TInstant deadline)
                : TBase(id, docsCount, storage, stat, deadline)
            {}

            virtual void ProcessData(const TTestData& data) override {
                Y_UNUSED(data);
                TBase::Storage.DoWrite(data);
            }

            virtual void UpdateTimes(const TDuration& time) override {
                Y_UNUSED(time);
                TBase::Stat.RegisterWrite();
            }
        };

        class TReaderTask : public TTaskWithDeadline {
            using TBase = TTaskWithDeadline;
        public:
            TReaderTask(ui32 id, ui32 docsCount, IStorageWrapper<TKey>& storage, TStat& stat, TInstant deadline)
                : TBase(id, docsCount, storage, stat, deadline)
            {}

            virtual void ProcessData(const TTestData& data) override {
                TBlob val = TBase::Storage.DoRead(data);
                CHECK_WITH_LOG(!val.Empty());
                if (val.Size() != data.Value.Size()) {
                    INFO_LOG << "key=" << data.Key << ";real=" << val.Size() << ";expected=" << data.Value.Size() << Endl;
                }
                CHECK_WITH_LOG(val.Size() == data.Value.Size());
            }

            virtual void UpdateTimes(const TDuration& time) override {
                Y_UNUSED(time);
                TBase::Stat.RegisterRead();
            }
        };

        const TTestData& GetData(ui32 id, ui32 doc, ui32 docsPerWorker) const {
            ui64 index = id * docsPerWorker + doc;
            CHECK_WITH_LOG(index < TestData.size());
            return TestData[index];
        }
    };
}
