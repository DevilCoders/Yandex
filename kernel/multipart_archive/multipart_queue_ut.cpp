#include "test_utils.h"

#include <kernel/multipart_archive/queue/queue_storage.h>
#include <kernel/multipart_archive/iterators/factory.h>

#include <util/ysaveload.h>
#include <util/thread/pool.h>
#include <util/random/random.h>

using namespace NRTYArchive;

Y_UNIT_TEST_SUITE(StorageSuite) {
    Y_UNIT_TEST(PartsStorageSimple) {
        InitLog();
        TFsPath arcPath("test_dummy_fat");
        Clear(arcPath);

        IArchivePart::TConstructContext context(IArchivePart::RAW, IDataAccessor::DIRECT_FILE, 3 * (1 + sizeof(ui32)));
        context.MapHeader = true;
        TMultipartStorage parts(arcPath, context);

        for (ui32 docId = 0; docId < 10; ++docId) {
            parts.AppendDocument(TBlob::FromString(ToString(docId)));
        }

        UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 4);
        parts.RemoveDocument(1, 0);
        parts.RemoveDocument(1, 1);
        parts.RemoveDocument(1, 2);
        UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 3);

        auto iter = parts.CreateCommonIterator(new TRawIteratorFactory());
        ui32 docid = 0;
        while (iter->IsValid()) {
            TBlob doc = iter->GetDocument();
            //           DEBUG_LOG << docid << " : " <<  TString((const char*)doc.Data(), doc.Size()) << Endl;
            UNIT_ASSERT_VALUES_EQUAL(TString((const char*)doc.Data(), doc.Size()), ToString(docid));
            iter->Next();
            if (docid == 2)
                docid = 5;
            docid++;
        }

        UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(false), 7);
        parts.Clear(0);
        UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(false), 0);
        UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 1);

        for (ui32 docId = 0; docId < 2; ++docId) {
            parts.AppendDocument(TBlob::FromString(ToString(docId)));
        }

        UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(false), 2);
        parts.Clear(0);
        TFsPath(arcPath.GetPath() + FAT_EXT).DeleteIfExists();
    }

    Y_UNIT_TEST(TestStorageRestart) {
        InitLog();
        TFsPath arcPath("test_storage");
        Clear(arcPath);

        IArchivePart::TConstructContext context(IArchivePart::RAW, IDataAccessor::DIRECT_FILE, 4 * (1 + sizeof(ui32)));
        context.MapHeader = true;
        {
            TMultipartStorage parts(arcPath, context);

            for (ui32 docId = 0; docId < 10; ++docId) {
                parts.AppendDocument(TBlob::FromString(ToString(docId)));
            }

            UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 3);
            parts.RemoveDocument(1, 2);
        }

        {
            TMultipartStorage parts(arcPath, context);
            UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 4);
            UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(false), 9);
        }

        TMultipartStorage parts(arcPath, context);
        UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 4);
        UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(false), 9);
        parts.Clear(0);
        TFsPath(arcPath.GetPath() + FAT_EXT).DeleteIfExists();
    }

    Y_UNIT_TEST(TestStoragePartCreationFails) {
        InitLog();
        TFsPath arcPath("test_storage");
        Clear(arcPath);

        IArchivePart::TConstructContext context(IArchivePart::RAW, IDataAccessor::DIRECT_FILE, 4 * (1 + sizeof(ui32)));
        context.MapHeader = true;
        {
            TMultipartStorage parts(arcPath, context);

            for (ui32 docId = 0; docId < 10; ++docId) {
                parts.AppendDocument(TBlob::FromString(ToString(docId)));
            }

            UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 3);
        }
        NFs::Remove(arcPath.GetPath() + PART_EXT + "2" + PART_HEADER_EXT);

        TMultipartStorage parts(arcPath, context);
        UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 3);
        parts.AppendDocument(TBlob::FromString("133"));
        UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 3);
        parts.Clear(0);
    }

    Y_UNIT_TEST(TestStorageBrokenPartHeader) {
        InitLog();
        TFsPath arcPath("test_storage");
        Clear(arcPath);

        IArchivePart::TConstructContext context(IArchivePart::RAW, IDataAccessor::DIRECT_FILE, 4 * (1 + sizeof(ui32)));
        context.MapHeader = true;
        {
            TMultipartStorage parts(arcPath, context);

            for (ui32 docId = 0; docId < 10; ++docId) {
                parts.AppendDocument(TBlob::FromString(ToString(docId)));
            }

            UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 3);
        }

        {
            TPartMetaSaver(GetPartMetaPath(arcPath, 0))->SetStage(TPartMetaInfo::OPENED);
            TPartMetaSaver(GetPartMetaPath(arcPath, 1))->SetStage(TPartMetaInfo::OPENED);
            TPartMetaSaver(GetPartMetaPath(arcPath, 2))->SetStage(TPartMetaInfo::BUILD);
        }
        TVector<ui32> newHeader;
        newHeader.push_back(-1);
        newHeader.push_back(1);
        newHeader.push_back(0);
        newHeader.push_back(0);
        newHeader.push_back(0);
        TFileOutput out(arcPath.GetPath() + PART_EXT + "2" + PART_HEADER_EXT);
        ::SaveArray(&out, newHeader.data(), newHeader.size());
        out.Flush();

        TMultipartStorage parts(arcPath, context);
        {
            TFileMappedArray<ui32> header2;
            header2.Init((arcPath.GetPath() + PART_EXT + "2" + PART_HEADER_EXT).data());
            UNIT_ASSERT_VALUES_EQUAL(header2.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 4);
            UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(false), 9);
            UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(true), 0);
            ui32 index = 0;
            for (auto i = parts.CreateCommonIterator(new TRawIteratorFactory()); i->IsValid(); i->Next()) {
                INFO_LOG << index << " " << i->GetDocid() << Endl;
                if (i->GetDocid() == (ui32)-1) {
                    UNIT_ASSERT(index == 8);
                } else {
                    UNIT_ASSERT(index != 8);
                }
                ++index;
            }

            parts.AppendDocument(TBlob::FromString("1"));
            parts.AppendDocument(TBlob::FromString("2"));
            UNIT_ASSERT_VALUES_EQUAL(parts.GetPartsCount(), 4);
            UNIT_ASSERT_VALUES_EQUAL(parts.GetDocsCount(false), 11);
        }
        parts.Clear(0);
        TFsPath(arcPath.GetPath() + FAT_EXT).DeleteIfExists();
    }
}

Y_UNIT_TEST_SUITE(QueueSuite) {
    Y_UNIT_TEST(TestStorageOpeningTime) {
        InitLog();
        TFsPath path("storage");
        Clear(path.GetPath());
        {
            TStorage storage(path, TStorage::TLimits(), TMultipartConfig());
            for (ui32 i = 0; i < 100000; ++i) {
                TBlob document = TBlob::FromString("document");
                storage.Put(document);
            }
        }
        TDuration commonRestartTime;
        {
            TInstant start = TInstant::Now();
            TStorage storage(path, TStorage::TLimits(), TMultipartConfig());
            TInstant finish = TInstant::Now();
            commonRestartTime = finish - start;
            INFO_LOG << "Restart time:" << commonRestartTime.MilliSeconds() << Endl;
        }

        TDuration restartOpened;
        {
            TVector<ui32> partIndexes;
            TMultipartStorage::FillPartsIndexes(path, partIndexes);
            for (ui32 i = 0; i < partIndexes.size(); ++i) {
                TFsPath headerMetaPath = GetPartMetaPath(path, partIndexes[i]);
                TPartMetaSaver(headerMetaPath)->SetStage(TPartMetaInfo::OPENED);
            }

            TInstant start = TInstant::Now();
            TStorage storage(path, TStorage::TLimits(), TMultipartConfig());
            TInstant finish = TInstant::Now();
            restartOpened = finish - start;
            INFO_LOG << "Restart time (opened):" << restartOpened.MilliSeconds() << Endl;
        }

        TDuration fullRepair;
        {
            TVector<ui32> partIndexes;
            TMultipartStorage::FillPartsIndexes(path, partIndexes);
            for (ui32 i = 0; i < partIndexes.size(); ++i) {
                TFsPath headerMetaPath = GetPartMetaPath(path, partIndexes[i]);
                TPartMetaSaver(headerMetaPath)->SetStage(TPartMetaInfo::BUILD);
            }

            TInstant start = TInstant::Now();
            TStorage storage(path, TStorage::TLimits(), TMultipartConfig());
            TInstant finish = TInstant::Now();
            fullRepair = finish - start;
            INFO_LOG << "Restart time (full repair):" << fullRepair.MilliSeconds() << Endl;
        }

        UNIT_ASSERT(2 * restartOpened.MilliSeconds() < fullRepair.MilliSeconds());
    }

    Y_UNIT_TEST(TestReadFromEmptyStorage) {
        InitLog();
        TStorage storage("sas", TStorage::TLimits(100), TMultipartConfig());
        TStorage::TDocument::TPtr doc = storage.Get(false);
        UNIT_ASSERT(!doc);
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 0);
    }

    Y_UNIT_TEST(TestPutGet) {
        InitLog();
        Clear("storage");
        TStorage storage("storage", TStorage::TLimits(30), TMultipartConfig());
        TBlob document = TBlob::FromString("document");

        for (ui32 i = 0; i < 100; ++i) {
            INFO_LOG << "Iteration " << i << Endl;
            storage.Put(document);
            storage.Put(document);
            UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 2);
            {
                auto doc1 = storage.Get(false);
                UNIT_ASSERT(!!doc1);
                auto doc2 = storage.Get(false);
                UNIT_ASSERT(!!doc2);
                auto doc3 = storage.Get(false);
                UNIT_ASSERT(!doc3);
                doc1->Ack();
                auto doc4 = storage.Get(false);
                UNIT_ASSERT(!doc4);
                doc2->Ack();
                auto doc5 = storage.Get(false);
                UNIT_ASSERT(!doc5);
            }
        }
    }

    Y_UNIT_TEST(TestRemoveEmptyStorageFiles) {
        InitLog();
        Clear("storage");
        {
            {
                TStorage storage("storage", TStorage::TLimits(300), TMultipartConfig());
                TBlob document = TBlob::FromString("document");
                for (ui32 i = 0; i < 100; ++i) {
                    storage.Put(document);
                }
                for (ui32 i = 0; i < 100; ++i) {
                    auto doc = storage.Get(false);
                    doc->Ack();
                }
            }

            UNIT_ASSERT_VALUES_EQUAL(TFsPath(TString::Join("storage", FAT_EXT)).Exists(), false);
        }
        {
            {
                TStorage storage("storage", TStorage::TLimits(300), TMultipartConfig());
                TBlob document = TBlob::FromString("document");
                for (ui32 i = 0; i < 100; ++i) {
                    storage.Put(document);
                }
                for (ui32 i = 0; i < 99; ++i) {
                    auto doc = storage.Get(false);
                    doc->Ack();
                }
            }

            UNIT_ASSERT_VALUES_EQUAL(TFsPath(TString::Join("storage", FAT_EXT)).Exists(), true);
        }
    }

    Y_UNIT_TEST(TestRemoveEmptyPartsOnStart) {
        InitLog();
        Clear("storage");
        {
            TStorage storage("storage", TStorage::TLimits(300), TMultipartConfig());
            TBlob document = TBlob::FromString("document");
            for (ui32 i = 0; i < 100; ++i)
                storage.Put(document);
        }
        TVector<ui32> hdr(100, Max<ui32>());
        IPartHeader::SaveRestoredHeader("storage.part.0.hdr", hdr);
        TPartMetaSaver(GetPartMetaPath("storage", 0))->SetStage(TPartMetaInfo::OPENED);
        TStorage storage("storage", TStorage::TLimits(300), TMultipartConfig());
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 0);
        UNIT_ASSERT_VALUES_EQUAL(storage.GetSizeInBytes(), 0);
    }

    Y_UNIT_TEST(TestClear) {
        InitLog();
        Clear("storage");
        TStorage storage("storage", TStorage::TLimits(30), TMultipartConfig());
        TVector<TStorage::TDocument::TPtr> docs;
        for (ui32 i = 0; i < 5; ++i) {
            storage.Put(TBlob::FromString(ToString(i)));
            TStorage::TDocument::TPtr doc = storage.Get(true);
            UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc->GetBlob().AsCharPtr(), doc->GetBlob().Size()), ToString(i));
            docs.push_back(doc);
        }
        docs[0]->Ack();
        docs[3]->Ack();
        docs[0].Drop();
        docs[3].Drop();
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 3);
        storage.Clear();
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 0);
        storage.Put(TBlob::FromString("aaa"));
        storage.Put(TBlob::FromString("bbb"));
        storage.Put(TBlob::FromString("ccc"));
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 3);
        docs[2].Drop();
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 3);
    }

    Y_UNIT_TEST(TestReadAllData) {
        InitLog();
        Clear("storage");
        TStorage storage("storage", TStorage::TLimits(1), TMultipartConfig());
        for (ui32 i = 0; i < 5; ++i) {
            storage.Put(TBlob::FromString(ToString(i)));
            TStorage::TDocument::TPtr doc = storage.Get(true);
            UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc->GetBlob().AsCharPtr(), doc->GetBlob().Size()), ToString(i));
            doc->Ack();
        }
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 0);
    }

    Y_UNIT_TEST(TestStopQueue) {
        struct TWaitThread: public IObjectInQueue {
            TWaitThread(TStorage& storage)
                : Storage(storage)
                , Finished(0) {}

            void Process(void*) override {
                Storage.WaitStopped();
                AtomicSet(Finished, 1);
            }

            bool IsFinished() const {
                return AtomicGet(Finished) == 1;
            }

        private:
            TStorage& Storage;
            TAtomic Finished;
        };

        InitLog();
        Clear("storage");
        TStorage storage("storage", TStorage::TLimits(10), TMultipartConfig());
        for (ui32 i = 0; i < 5; ++i) {
            storage.Put(TBlob::FromString(ToString(i)));
        }
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 5);

        TStorage::TDocument::TPtr doc = storage.Get(false);
        UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc->GetBlob().AsCharPtr(), doc->GetBlob().Size()), ToString(0));
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 5);

        storage.Stop();
        UNIT_ASSERT(!storage.Get(false));
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 5);


        TThreadPool waitQ;
        waitQ.Start(1);
        TWaitThread wt(storage);
        UNIT_ASSERT(waitQ.Add(&wt));
        UNIT_ASSERT(!wt.IsFinished());
        doc = nullptr;
        Sleep(TDuration::Seconds(1));
        UNIT_ASSERT(wt.IsFinished());
    }

    Y_UNIT_TEST(TestLimits) {
        InitLog();
        bool wasErr = false;
        try {
            Clear("storage");
            TStorage storage("storage", TStorage::TLimits(1), TMultipartConfig());
            for (ui32 i = 0; i < 2; ++i)
                storage.Put(TBlob::FromString(ToString(i)));
        } catch (...) {
            wasErr = true;
        }
        UNIT_ASSERT(wasErr);
        wasErr = false;
        try {
            TStorage storage("storage", TStorage::TLimits(10, 1), TMultipartConfig());
            storage.Put(TBlob::FromString("123"));
        } catch (...) {
            wasErr = true;
        }
        UNIT_ASSERT(wasErr);
        {
            TStorage storage("storage", TStorage::TLimits(10, Max<ui64>(), 1), TMultipartConfig());
            for (ui32 i = 0; i < 5; ++i)
                storage.Put(TBlob::FromString(ToString(i)));
            TStorage::TDocument::TPtr doc1 = storage.Get(false);
            TStorage::TDocument::TPtr doc2 = storage.Get(false);
            UNIT_ASSERT(!!doc1);
            UNIT_ASSERT(!doc2);
        }
    }

    Y_UNIT_TEST(TestNotFlushedSimple) {
        InitLog();
        Clear("storage_flush");
        {
            TStorage storage("storage_flush", TStorage::TLimits(10), TMultipartConfig());
            storage.Put(TBlob::FromString("123"));
            storage.Put(TBlob::FromString("456"));
        }
        {
            TFile f("storage_flush.part.0", WrOnly | CreateAlways);
            TPartMetaSaver(GetPartMetaPath("storage", 0))->SetStage(TPartMetaInfo::BUILD);
        }
        TStorage storage("storage_flush", TStorage::TLimits(100), TMultipartConfig());
        TStorage::TDocument::TPtr doc1 = storage.Get(false);
        TStorage::TDocument::TPtr doc2 = storage.Get(false);
        UNIT_ASSERT(!doc1);
        UNIT_ASSERT(!doc2);
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 0);
    }

    Y_UNIT_TEST(TestNotFlushed) {
        InitLog();
        Clear("storage_flush");
        {
            TStorage storage("storage_flush", TStorage::TLimits(10), TMultipartConfig());
            storage.Put(TBlob::FromString("123"));
            storage.Put(TBlob::FromString("456"));
        }
        {
            TStorage storage("storage_flush", TStorage::TLimits(10), TMultipartConfig());
            storage.Put(TBlob::FromString("12"));
            storage.Put(TBlob::FromString("45"));
            UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 4);
            TStorage::TDocument::TPtr doc1 = storage.Get(false);
            TStorage::TDocument::TPtr doc2 = storage.Get(false);
            UNIT_ASSERT(doc1);
            UNIT_ASSERT(doc2);
            doc1->Ack();
            doc2->Ack();
            doc1.Drop();
            doc2.Drop();
            UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 2);
        }
        {
            TFile f("storage_flush.part.1", WrOnly | CreateAlways);
            TFsPath("~storage_flush.part.1.hdr").Touch();
        }
        TStorage storage("storage_flush", TStorage::TLimits(100), TMultipartConfig());
        TStorage::TDocument::TPtr doc1 = storage.Get(false);
        TStorage::TDocument::TPtr doc2 = storage.Get(false);
        UNIT_ASSERT(!doc1);
        UNIT_ASSERT(!doc2);
        UNIT_ASSERT_VALUES_EQUAL(storage.GetDocsCount(), 0);
    }
}
