#include "multipart.h"
#include "archive_fat.h"
#include "test_utils.h"
#include "archive_test_base.h"

#include <kernel/multipart_archive/abstract/globals.h>
#include <kernel/multipart_archive/config/config.h>
#include <kernel/multipart_archive/iterators/part_iterator.h>

#include <util/ysaveload.h>
#include <util/stream/file.h>

using namespace NRTYArchive;
using namespace NRTYArchiveTest;


Y_UNIT_TEST_SUITE_IMPL(ArchiveIteratorsTests, TMultipartArchiveTestBase) {

    Y_UNIT_TEST(TestEmptyArchiveIterator) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(10, data);
        TArchiveConstructor::ConstructDirect(data, 0);

        TArchivePartThreadSafe::Remove("test", 0);
        TMultipartConfig config;
        TEngine::TPtr archive = TEngine::Create("test", config);
        TMultipartArchive::TIterator::TPtr it = archive->CreateIterator();
        UNIT_ASSERT(!it->IsValid());

        auto docIds = archive->GetDocIds();
        UNIT_ASSERT(docIds.empty());
    }

    IArchivePart::TConstructContext PrepareContext(IArchivePart::TType compression, IDataAccessor::TType accessor) {
        IArchivePart::TConstructContext context(compression, accessor);
        if (compression == IArchivePart::COMPRESSED_EXT) {
            context.Compression.ExtParams.CodecName = "zstd08d-1";
            context.Compression.ExtParams.LearnSize = 100;
            context.Compression.ExtParams.BlockSize = 100;
        }
        return context;
    }

    static void TestIterator(IArchivePart::TType compression, bool close) {
        const TFsPath arcPrefix(ARCHIVE_PREFIX);
        TMultipartArchiveTestBase::Clear();
        {
            TPartCallBack cb;
            auto context = PrepareContext(compression, IDataAccessor::DIRECT_FILE);
            TArchiveManager mgr(context);
            TArchivePartThreadSafe::TPtr part = new TArchivePartThreadSafe(arcPrefix, 0, &mgr, true, cb);

            TFATMultipart FAT(GetFatPath(arcPrefix), 200, TMemoryMapCommon::oRdWr);
            for (ui32 docid = 0; docid < 200; ++docid) {
                TBlob document = TBlob::FromString(ToString(docid));
                ui64 offset = PutDocumentWithCheck(*part, document, docid);
                FAT.Set(docid, TPosition(offset, 0));
            }

            if (close) {
                part->Close();
            }

            auto snapshotDocIds = GetIdsFromSnapshot(FAT.GetSnapshot());
            UNIT_ASSERT_VALUES_EQUAL(snapshotDocIds.size(), 200);
            for (size_t i = 0; i < snapshotDocIds.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(snapshotDocIds[i], i);
            }

            TOffsetsPtr offsets = TOffsetsIterator::CreateOffsets(0, &FAT);
            TAutoPtr<TOffsetsIterator> iter(new TOffsetsIterator(part, &FAT, offsets));
            ui32 docid = 0;
            while (iter->IsValid()) {
                TBlob doc = iter->GetDocument();
                //           Cout << iter->GetDocId() << Endl;
                UNIT_ASSERT_VALUES_EQUAL(iter->GetDocId(), docid);
                UNIT_ASSERT_VALUES_EQUAL(TString((const char*)doc.Data(), doc.Size()), ToString(docid));
                iter->Next();
                if (!close && docid == 10) {
                    part->Close();
                }
                docid++;
            }
        }
    }

    Y_UNIT_TEST(TestClosedPartIterator) {
        TestIterator(IArchivePart::RAW, true);
        TestIterator(IArchivePart::COMPRESSED, true);
        TestIterator(IArchivePart::COMPRESSED_EXT, true);
    }

    static void TestIteratorRemovedDocs(IArchivePart::TType compression) {
        const TString arcPrefix(ARCHIVE_PREFIX);
        TMultipartArchiveTestBase::Clear();
        {
            TPartCallBack cb;
            auto context = PrepareContext(compression, IDataAccessor::DIRECT_FILE);
            TArchiveManager mgr(context);
            TArchivePartThreadSafe::TPtr part = new TArchivePartThreadSafe(arcPrefix, 0, &mgr, true, cb);

            TFATMultipart FAT(GetFatPath(arcPrefix), 0, TMemoryMapCommon::oRdWr);

            ui64 offset = PutDocumentWithCheck(*part, TBlob::FromString("0"), 0);
            FAT.Set(0, TPosition(offset, 1));
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(0).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("1"), 1);
            FAT.Set(1, TPosition(offset, 1));
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(1).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("2"), 2);
            FAT.Set(2, TPosition(offset, 0));
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(2).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("3"), 3);
            FAT.Set(3, TPosition(offset, 0));
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(3).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("4"), 4);
            FAT.Set(4, TPosition(offset, 0));
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(4).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("5"), 5);
            FAT.Set(5, TPosition(offset, 0));
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(5).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("6"), 6);
            offset = PutDocumentWithCheck(*part, TBlob::FromString("7"), 7);
            FAT.Set(7, TPosition::Removed());

            offset = PutDocumentWithCheck(*part, TBlob::FromString("8"), 6);
            FAT.Set(6, TPosition(offset, 0)); // "8"
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(6).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("9"), 9);
            FAT.Set(9, TPosition(offset, 0));
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(9).GetOffset(), offset);

            offset = PutDocumentWithCheck(*part, TBlob::FromString("10"), 8);
            FAT.Set(8, TPosition(offset, 0)); // "10"
            UNIT_ASSERT_VALUES_EQUAL(FAT.Get(8).GetOffset(), offset);

            part->Close();

            ui32 resultId[] = { 2,3,4,5,6,9,8 };
            TString resultKeys[] = { "2","3","4","5","8","9","10" };

            TOffsetsPtr offsets = TOffsetsIterator::CreateOffsets(0, &FAT);
            TAutoPtr<TOffsetsIterator> iter(new TOffsetsIterator(part, &FAT, offsets));
            ui32 docid = 0;
            while (iter->IsValid()) {
                TBlob doc = iter->GetDocument();
                UNIT_ASSERT_VALUES_EQUAL(iter->GetDocId(), resultId[docid]);
                UNIT_ASSERT_VALUES_EQUAL(TString((const char*)doc.Data(), doc.Size()), resultKeys[docid]);
                iter->Next();
                docid++;
            }

            constexpr ui32 expectedDocIds[] = {0, 1, 2, 3, 4, 5, 6, 8, 9};
            auto snapshotDocIds = GetIdsFromSnapshot(FAT.GetSnapshot());
            UNIT_ASSERT_VALUES_EQUAL(snapshotDocIds.size(), std::extent_v<decltype(expectedDocIds)>);
            for (size_t i = 0; i < snapshotDocIds.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(snapshotDocIds[i], expectedDocIds[i]);
            }
        }
    }

    Y_UNIT_TEST(TestClosedPartIteratorRemovedDocs) {
        TestIteratorRemovedDocs(IArchivePart::RAW);
        TestIteratorRemovedDocs(IArchivePart::COMPRESSED);
        TestIteratorRemovedDocs(IArchivePart::COMPRESSED_EXT);
    }

    Y_UNIT_TEST(TestNotClosedPartIterator) {
        TestIterator(IArchivePart::RAW, false);
        TestIterator(IArchivePart::COMPRESSED, false);
        TestIterator(IArchivePart::COMPRESSED_EXT, false);
    }

    static void TestTwoIteratorsBase(IArchivePart::TType compression) {
        const TString arcPrefix(ARCHIVE_PREFIX);
        TMultipartArchiveTestBase::Clear();
        {
            TPartCallBack cb;
            auto context = PrepareContext(compression, IDataAccessor::DIRECT_FILE);
            TArchiveManager mgr(context);
            TArchivePartThreadSafe::TPtr part = new TArchivePartThreadSafe(arcPrefix, 0, &mgr, true, cb);

            TFATMultipart FAT(GetFatPath(arcPrefix), 20, TMemoryMapCommon::oRdWr);
            for (ui32 docid = 0; docid < 20; ++docid) {
                TBlob document = TBlob::FromString(ToString(docid));
                ui64 offset = PutDocumentWithCheck(*part, document, docid);
                FAT.Set(docid, TPosition(offset, 0));
            }

            part->Close();

            TOffsetsPtr offsets = TOffsetsIterator::CreateOffsets(0, &FAT);
            TAutoPtr<TOffsetsIterator>  iter1(new TOffsetsIterator(part, &FAT, offsets));
            TAutoPtr<TOffsetsIterator>  iter2(new TOffsetsIterator(part, &FAT, offsets));
            iter2->Next();
            iter2->Next();

            ui32 docid = 0;
            while (iter1->IsValid()) {
                TBlob doc = iter1->GetDocument();
                //            Cout << "1: " << iter1->GetDocId() << Endl;
                UNIT_ASSERT_VALUES_EQUAL(iter1->GetDocId(), docid);
                UNIT_ASSERT_VALUES_EQUAL(TString((const char*)doc.Data(), doc.Size()), ToString(docid));
                if (docid < 18) {
                    //                Cout << "2: " << iter2->GetDocId() << Endl;
                    TBlob doc2 = iter2->GetDocument();
                    UNIT_ASSERT_VALUES_EQUAL(iter2->GetDocId(), docid + 2);
                    UNIT_ASSERT_VALUES_EQUAL(TString((const char*)doc2.Data(), doc2.Size()), ToString(docid + 2));
                    iter2->Next();
                }
                iter1->Next();
                docid++;
            }
        }
    }

    Y_UNIT_TEST(TestTwoIterators) {
        TestTwoIteratorsBase(IArchivePart::RAW);
        TestTwoIteratorsBase(IArchivePart::COMPRESSED);
        TestTwoIteratorsBase(IArchivePart::COMPRESSED_EXT);
    }

    static void TestRawIteratorsBase(IArchivePart::TType compression, bool close) {
        const TString arcPrefix(ARCHIVE_PREFIX);
        TMultipartArchiveTestBase::Clear();
        {
            TPartCallBack cb;
            auto context = PrepareContext(compression, IDataAccessor::DIRECT_FILE);
            TArchiveManager mgr(context);
            TArchivePartThreadSafe::TPtr part = new TArchivePartThreadSafe(arcPrefix, 0, &mgr, true, cb);

            TFATMultipart FAT(GetFatPath(arcPrefix), 200, TMemoryMapCommon::oRdWr);
            for (ui32 docid = 0; docid < 200; ++docid) {
                TBlob document = TBlob::FromString(ToString(docid));
                ui64 offset = PutDocumentWithCheck(*part, document, docid);
                FAT.Set(docid,TPosition(offset, 0));
            }

            if (close)
                part->Close();

            TAutoPtr<TRawIterator> iter(new TRawIterator(part));
            ui32 docid = 0;
            while (iter->IsValid()) {
                TBlob doc = iter->GetDocument();
                //            DEBUG_LOG << compression << " : " <<  TString((const char*)doc.Data(), doc.Size()) << Endl;
                UNIT_ASSERT_VALUES_EQUAL(TString((const char*)doc.Data(), doc.Size()), ToString(docid));
                iter->Next();
                if (!close && docid == 10) {
                    part->Close();
                }
                docid++;
            }
        }
    }

    Y_UNIT_TEST(TestRawIterator) {
        TestRawIteratorsBase(IArchivePart::RAW, true);
        TestRawIteratorsBase(IArchivePart::COMPRESSED, true);
        TestRawIteratorsBase(IArchivePart::COMPRESSED_EXT, true);
        TestRawIteratorsBase(IArchivePart::RAW, false);
        TestRawIteratorsBase(IArchivePart::COMPRESSED, false);
        TestRawIteratorsBase(IArchivePart::COMPRESSED_EXT, false);
    }

    Y_UNIT_TEST(TestIteratorOnFlush) {
        TFsPath arcPath("test_archive");
        auto archive = TEngine::Create(arcPath, TMultipartConfig(), 200);

        for (ui32 docId = 0; docId < 200; ++docId) {
            archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
        }

        ui32 docId = 0;
        archive->Flush();
        for (auto it = archive->CreateIterator(true); it->IsValid(); it->Next()) {
            UNIT_ASSERT_VALUES_EQUAL(it->GetDocid(), docId);
            TBlob doc = it->GetDocument();
            UNIT_ASSERT_VALUES_EQUAL(TString(doc.AsCharPtr(), doc.Size()), ToString(docId));
            ++docId;
        }
    }

    Y_UNIT_TEST(TestIterateAfterNotFinishedPut) {
        TFsPath arcPath("test_archive");

        TVector<ui32> newHeader;
        {
            auto archive = TEngine::Create(arcPath, TMultipartConfig(), 0);

            for (ui32 docId = 0; docId < 6; ++docId) {
                archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
                newHeader.push_back(docId);
            }
            archive->RemoveDocument(4);
        }

        newHeader[4] = 2;
        TFsPath headerPath(arcPath.GetPath() + PART_EXT + "0" + PART_HEADER_EXT);
        TFileOutput out(headerPath.GetPath());
        ::SaveArray(&out, newHeader.data(), newHeader.size());
        out.Flush();

       auto archive = TEngine::Create(arcPath, TMultipartConfig());

        ui32 docid = 0;
        for (auto it = archive->CreateIterator(); it->IsValid(); it->Next()) {
            UNIT_ASSERT_VALUES_EQUAL(it->GetDocid(), docid);
            TBlob doc = it->GetDocument();
            UNIT_ASSERT_VALUES_EQUAL(TString(doc.AsCharPtr(), doc.Size()), ToString(docid));
            docid++;
            if (docid == 4)
                docid++;
        }

    }
}
