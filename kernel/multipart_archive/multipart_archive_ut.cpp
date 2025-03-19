#include "multipart.h"
#include "archive_fat.h"
#include "test_utils.h"
#include "archive_test_base.h"
#include "owner.h"

#include <kernel/multipart_archive/abstract/globals.h>
#include <kernel/multipart_archive/config/config.h>

#include <util/generic/buffer.h>
#include <util/ysaveload.h>
#include <util/stream/file.h>
#include <util/system/fstat.h>
#include <util/system/info.h>


using namespace NRTYArchive;
using namespace NRTYArchiveTest;

TString GetStr(const TBlob& data) {
    return TString(data.AsCharPtr(), data.Size());
}


Y_UNIT_TEST_SUITE_IMPL(ArchiveComponentsTests, TMultipartArchiveTestBase) {
    Y_UNIT_TEST(TestMappedGuard) {
        EnableGlobalIndexFileMapping();
        UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 0);
        TFsPath fileName = TArchiveOwner::CreateKey("xxx");
        {
            TFixedBufferFileOutput out(fileName);
            out << "xxx";
        }
        {
            TMappedFileGuard::TPtr g1;
            {
                TMappedFileGuard::TPtr g2(new TMappedFileGuard(fileName));
                UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 1);
                g1 = g2;
                UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 1);
            }
            UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 1);
        }
        UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 0);

    }

    Y_UNIT_TEST(TestLockFAT) {
        TMultipartConfig config;
        config.LockFAT = true;
        UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 0);
        {
            auto archive = TArchiveOwner::Create("test", config);
            UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 1);
            archive->PutDocument(TBlob::FromString("test"), 5);
            UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 1);
        }
        UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 0);
        {
            auto archive = TArchiveOwner::Create("test", config);
            UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 1);
        }
        UNIT_ASSERT_VALUES_EQUAL(NMappedFiles::GetLockedFilesCount() , 0);
    }

    Y_UNIT_TEST(TestTPosition) {
        {
           TPosition posN(0, 0);
            ui64 part = ((ui64)1 << 23) - 1;
            posN.SetPart(part);

            UNIT_ASSERT_EQUAL(part, posN.GetPart());

            ui64 oft = ((ui64)1 << 40) - 1;
            posN.SetOffset(oft);
            UNIT_ASSERT_EQUAL(oft, posN.GetOffset());

            UNIT_ASSERT_EQUAL(false, posN.IsRemoved());
            posN.SetRemoved();
            UNIT_ASSERT_EQUAL(true, posN.IsRemoved());
        }

        {
            auto pos = TPosition::Removed();
            UNIT_ASSERT_EQUAL(pos.IsRemoved(), true);
        }

        {
            TPosition posN(0, 0);
            ui64 part = ((ui64)1 << 23) - 10;
            posN.SetPart(part);

            UNIT_ASSERT_EQUAL(part, posN.GetPart());

            ui64 oft = ((ui64)1 << 40) - 10;
            posN.SetOffset(oft);
            UNIT_ASSERT_EQUAL(oft, posN.GetOffset());

            UNIT_ASSERT_EQUAL(false, posN.IsRemoved());
            posN.SetRemoved();
            UNIT_ASSERT_EQUAL(true, posN.IsRemoved());
        }
    }

    Y_UNIT_TEST(ParsePartName) {
        const TString arcPrefix(ARCHIVE_PREFIX);
        {
            const TString partFile = arcPrefix + ".part.13";
            ui32 index = TArchivePartThreadSafe::ParsePartFromFileName(partFile, arcPrefix);
            UNIT_ASSERT_VALUES_EQUAL(index, 13);
        }
        {
            const TString partFile = arcPrefix + ".part.13.hdr";
            ui32 index = TArchivePartThreadSafe::ParsePartFromFileName(partFile, arcPrefix);
            UNIT_ASSERT_VALUES_EQUAL(index, Max<ui32>());
        }
        {
            const TString partFile = arcPrefix + ".partsdsd";
            ui32 index = TArchivePartThreadSafe::ParsePartFromFileName(partFile, arcPrefix);
            UNIT_ASSERT_VALUES_EQUAL(index, Max<ui32>());
        }
        {
            const TString partFile = arcPrefix + ".part.sdsd233";
            ui32 index = TArchivePartThreadSafe::ParsePartFromFileName(partFile, arcPrefix);
            UNIT_ASSERT_VALUES_EQUAL(index, Max<ui32>());
        }
        {
            const TString partFile = arcPrefix + "233";
            ui32 index = TArchivePartThreadSafe::ParsePartFromFileName(partFile, arcPrefix);
            UNIT_ASSERT_VALUES_EQUAL(index, Max<ui32>());
        }
    }

    Y_UNIT_TEST(TestPartHeader) {
        TFsPath headerPath("test_header");
        NFs::Remove(headerPath);
        {
            TPartHeader header(headerPath, CreateAlways);
            for (ui32 docid = 0; docid < 20; ++docid) {
                header.PushDocument(docid);
            }

            UNIT_ASSERT_VALUES_EQUAL(header.GetDocsCount(), 20);
            UNIT_ASSERT_VALUES_EQUAL(header.GetPath().Exists(), true);
            header.Close();
        }
        TPartHeader header1(headerPath, RdOnly);
        UNIT_ASSERT_VALUES_EQUAL(header1.GetDocsCount(), 20);

        NFs::Remove(headerPath);
    }

    Y_UNIT_TEST(TestOnePart) {
        const TFsPath arcPrefix(ARCHIVE_PREFIX);
        {
            TPartCallBack cb;
            IArchivePart::TConstructContext context(IArchivePart::RAW, IDataAccessor::DIRECT_FILE, 9 * (1 + sizeof(ui32)));
            TArchiveManager mgr(context);
            TArchivePartThreadSafe part(arcPrefix, 0, &mgr, true, cb);

            for (ui32 docid = 0; docid < 20; ++docid) {
                TBlob document = TBlob::FromString(ToString(docid));
                PutDocumentWithCheck(part, document, docid);
                if (part.IsFull()) {
                    part.Close();
                    part.CloseImpl();
                    break;
                }
            }

            UNIT_ASSERT_VALUES_EQUAL(part.GetDocsCount(), 9);
            UNIT_ASSERT_VALUES_EQUAL(GetPartHeaderPath(part.GetPath()).Exists(), true);
            UNIT_ASSERT_VALUES_EQUAL(part.GetPath().Exists(), true);

            TFileMappedArray<ui32> header;
            header.Init(GetPartHeaderPath(part.GetPath()).GetPath().data());
            UNIT_ASSERT_VALUES_EQUAL(header.size(), 9);
            for (ui32 docid = 0; docid < 9; ++docid) {
                UNIT_ASSERT_VALUES_EQUAL(header[docid], docid);
            }
        }
    }

    static void CheckFull(IArchivePart::TType type, size_t nDocs) {
        constexpr size_t partLimit = 1000;
        constexpr size_t docSize = 100;
        TPartCallBack cb;
        IArchivePart::TConstructContext context(type, IDataAccessor::FILE, partLimit);
        TArchiveManager mgr(context);
        {
            TArchivePartThreadSafe part("test", 0, &mgr, true, cb);

            TArchiveDataGen::TDataSet data;
            TArchiveConstructor::GenerateData(nDocs, data, docSize);

            UNIT_ASSERT(!part.IsFull());
            ui64 offset = 0;
            for (auto&& doc : data) {
                if (part.IsFull()) {
                    offset = part.TryPutDocument(doc.Value, doc.Key);
                    break;
                }
                offset = PutDocumentWithCheck(part, doc.Value, doc.Key);
            }
            UNIT_ASSERT_VALUES_EQUAL(offset, TPosition::InvalidOffset);
            UNIT_ASSERT(part.IsFull());
            UNIT_ASSERT(part.GetSizeInBytes() > partLimit);
        }

        TArchivePartThreadSafe::Remove("test", 0);
    }

    Y_UNIT_TEST(IsPartFullCompressedExt) {
        CheckFull(IArchivePart::COMPRESSED_EXT, 400);
    }

    Y_UNIT_TEST(IsPartFullCompressed) {
        CheckFull(IArchivePart::COMPRESSED, 400);
    }

    Y_UNIT_TEST(IsPartFullRaw) {
        CheckFull(IArchivePart::RAW, 50);
    }
}


Y_UNIT_TEST_SUITE_IMPL(ArchiveAppend, TMultipartArchiveTestBase) {
    Y_UNIT_TEST(TestAppendSimple) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(100, data);

        auto source = TArchiveConstructor::ConstructDirect(data, 0, "source");

        TArchiveDataGen::TDataSet data1;
        auto dest = TArchiveConstructor::ConstructDirect(data1, 0, "dest");

        dest->Append(*source.Get());

        UNIT_ASSERT_VALUES_EQUAL(source->GetDocsCount(false), dest->GetDocsCount(false));

        for (auto&& doc : data) {
            TBlob blob = dest->GetDocument(doc.Key);
            UNIT_ASSERT_VALUES_EQUAL(GetStr(blob), GetStr(doc.Value));
        }
    }

    class TRemoveTask: public IObjectInQueue {
    public:
        TRemoveTask(TArchiveOwner::TPtr archive, TAtomic& removed)
            : Archive(archive)
            , RemovedDocsCount(removed)
        {}

        void Process(void*) override {
            ui32 docsCount = Archive->GetDocsCount(true);
            for (ui32 i = 0; i < docsCount; ++i) {
                Sleep(TDuration::MilliSeconds(10));
                ui32 docId = RandomNumber<ui32>(docsCount);
                if (Archive->RemoveDocument(docId)) {
                    AtomicIncrement(RemovedDocsCount);
                }
            }
        }

    private:
        TArchiveOwner::TPtr Archive;
        TAtomic& RemovedDocsCount;
    };

    Y_UNIT_TEST(TestAppendIntensive) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(500, data);

        auto source = TArchiveConstructor::ConstructDirect(data, 500, "source");

        TArchiveDataGen::TDataSet dataEmpty;
        auto dest = TArchiveConstructor::ConstructDirect(dataEmpty, 0, "dest");

        TAtomic removed(0);
        TThreadPool removeThread;
        removeThread.Start(2);
        CHECK_WITH_LOG(removeThread.AddAndOwn(MakeHolder<TRemoveTask>(source, removed)));
        dest->Append(*source.Get());

        INFO_LOG << source->GetDocsCount(false) << Endl;
        INFO_LOG << dest->GetDocsCount(false) << Endl;
        UNIT_ASSERT_VALUES_EQUAL(source->GetDocsCount(false), source->GetDocsCount(true) - AtomicGet(removed));
        UNIT_ASSERT(dest->GetDocsCount(false) >= source->GetDocsCount(false));
    }

    Y_UNIT_TEST(TestAppendForDetach) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(100, data, 100);
        auto archive = TArchiveConstructor::ConstructDirect(data, 4, "test_archive_append");
        TArchiveDataGen::TDataSet data1;
        auto archive2 = TArchiveConstructor::ConstructDirect(data1, 0, "test_archive_append_2");

        TMultipartArchive::TRemap remap;
        remap[0] = 0;
        remap[1] = 1;
        remap[2] = 2;
        remap[3] = 6;
        remap[5] = 98;

        archive2->Append(*archive.Get(), &remap);

        UNIT_ASSERT_VALUES_EQUAL(archive2->GetPartsCount(), 3);
        UNIT_ASSERT_VALUES_EQUAL(archive2->GetDocsCount(true), 6);
        UNIT_ASSERT_VALUES_EQUAL(archive2->GetDocsCount(false), 5);

        for (auto&& docId : remap) {
            TBlob doc = archive2->GetDocument(docId.first);
            TBlob required = data[docId.second].Value;
            UNIT_ASSERT_VALUES_EQUAL(GetStr(doc), GetStr(required));
        }

        UNIT_ASSERT_VALUES_EQUAL(GetStr(archive2->GetDocument(4)), "");
    }

    Y_UNIT_TEST(TestAppend) {
        TArchiveDataGen::TDataSet data;
        TArchiveConstructor::GenerateData(20, data, 100);
        auto archive = TArchiveConstructor::ConstructDirect(data, 2, "test_archive_append");

        TArchiveDataGen::TDataSet data2;
        TArchiveConstructor::GenerateData(20, data2, 100);
        auto archive2 = TArchiveConstructor::ConstructDirect(data2, 2, "test_archive_append_2");

        TMultipartArchive::TRemap remap;
        for (ui32 docId = 0; docId < data2.size(); ++docId) {
            remap[docId + data.size()] = docId;
        }

        TSet<ui32> docsForRemove = {14, 18, 17, 19};
        for (auto&& doc : docsForRemove) {
            archive2->RemoveDocument(doc);
        }

        UNIT_ASSERT_VALUES_EQUAL(archive2->GetPartsCount(), 3);
        UNIT_ASSERT_VALUES_EQUAL(archive2->GetDocsCount(false), 16);

        archive->Append(*archive2.Get(), &remap, false);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 5);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 36);

        for (auto&& doc : data) {
            TBlob blob = archive->GetDocument(doc.Key);
            UNIT_ASSERT_VALUES_EQUAL(GetStr(blob), GetStr(doc.Value));
        }

        for (auto&& docId : remap) {
            TBlob doc = archive->GetDocument(docId.first);

            if (docsForRemove.contains(docId.second)) {
                UNIT_ASSERT_VALUES_EQUAL(GetStr(doc), "");
            } else {
                TBlob required = data2[docId.second].Value;
                UNIT_ASSERT_VALUES_EQUAL(GetStr(doc), GetStr(required));
            }
        }
    }
}


Y_UNIT_TEST_SUITE_IMPL(ArchiveRepairSuite, TMultipartArchiveTestBase) {
    Y_UNIT_TEST(TestArchiveRepair) {
        TFsPath arcPath("test_archive");

        ui32 docsCount = 10000;
        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;
        config.WritableThreadsCount = 3;

        auto archive = TEngine::Create(arcPath, config, docsCount);

        TArchiveDataGen dataGenerator(0, true);
        TArchiveDataGen::TDataSet data;
        dataGenerator.Generate(docsCount, data);

        for (ui32 i = 0; i < docsCount; ++i) {
            archive->PutDocument(data[i].Value, data[i].Key);
            if (i % 5 == 0) {
                archive->RemoveDocument(data[i].Key);
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 3);
        ui32 finalDocsCount = archive->GetDocsCount(false);

        archive.Drop();

        for (ui32 index = 0; index < docsCount; ++index) {
            TFsPath metaFile = GetPartMetaPath(GetPartPath(arcPath, index));
            TPartMetaSaver(metaFile)->SetStage(TPartMetaInfo::BUILD);
        }

        CHECK_WITH_LOG(!TEngine::Check(arcPath));
        TEngine::Repair(arcPath, config);
        CHECK_WITH_LOG(TEngine::Check(arcPath));

        archive.Reset(TEngine::Create(arcPath, config, docsCount));

        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(true), docsCount);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), finalDocsCount);
    }

    Y_UNIT_TEST(TestRestoreHeader) {
        TFsPath arcPath("test_archive");

        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;
        config.PartSizeLimit = 3 * (1 + sizeof(ui32));

        auto archive = TEngine::Create(arcPath, config, 0);

        for (ui32 docId = 0; docId < 9; ++docId) {
            archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
        }

        archive->RemoveDocument(2);
        archive->RemoveDocument(4);
        archive->RemoveDocument(5);
        archive->RemoveDocument(6);

        archive.Drop();

        UNIT_ASSERT(NFs::Remove(arcPath.GetPath() + PART_EXT + "0" + PART_HEADER_EXT));
        UNIT_ASSERT(NFs::Remove(arcPath.GetPath() + PART_EXT + "1" + PART_HEADER_EXT));
        UNIT_ASSERT(NFs::Remove(arcPath.GetPath() + PART_EXT + "2" + PART_HEADER_EXT));


        UNIT_ASSERT(!TEngine::Check(arcPath));
        TEngine::Repair(arcPath, config);
        UNIT_ASSERT(TEngine::Check(arcPath));

        auto readableArc = TEngine::Create(arcPath, config);

        UNIT_ASSERT_VALUES_EQUAL(readableArc->GetPartsCount(), 4);
        UNIT_ASSERT_VALUES_EQUAL(readableArc->GetDocsCount(false), 5);
        readableArc.Drop();

        {
            TFileMappedArray<ui32> header;
            header.Init((arcPath.GetPath() + PART_EXT + "0" + PART_HEADER_EXT).data());
            UNIT_ASSERT_VALUES_EQUAL(header.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(header[0], 0);
            UNIT_ASSERT_VALUES_EQUAL(header[1], 1);
            UNIT_ASSERT_VALUES_EQUAL(header[2], (ui32)-1);
        }

        {
            TFileMappedArray<ui32> header;
            header.Init((arcPath.GetPath() + PART_EXT + "1" + PART_HEADER_EXT).data());
            UNIT_ASSERT_VALUES_EQUAL(header.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(header[0], 3);
            UNIT_ASSERT_VALUES_EQUAL(header[1], (ui32)-1);
            UNIT_ASSERT_VALUES_EQUAL(header[2], (ui32)-1);
        }

        {
            TFileMappedArray<ui32> header;
            header.Init((arcPath.GetPath() + PART_EXT + "2" + PART_HEADER_EXT).data());
            UNIT_ASSERT_VALUES_EQUAL(header.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(header[0], (ui32)-1);
            UNIT_ASSERT_VALUES_EQUAL(header[1], 7);
            UNIT_ASSERT_VALUES_EQUAL(header[2], 8);
        }

        readableArc = TEngine::Create(arcPath, config);
        auto iter = readableArc->CreateIterator();
        UNIT_ASSERT_VALUES_EQUAL(iter->GetDocid(), 0);
        iter->Next();
        UNIT_ASSERT_VALUES_EQUAL(iter->GetDocid(), 1);
        iter->Next();
        UNIT_ASSERT_VALUES_EQUAL(iter->GetDocid(), 3);
        iter->Next();
        UNIT_ASSERT_VALUES_EQUAL(iter->GetDocid(), 7);
        iter->Next();
        UNIT_ASSERT_VALUES_EQUAL(iter->GetDocid(), 8);
        iter->Next();
        UNIT_ASSERT_VALUES_EQUAL(iter->IsValid(), false);
        readableArc.Drop();

        TEngine::Remove(arcPath);
    }

    Y_UNIT_TEST(TestRemoveEmptyPart) {
        TFsPath arcPath("test_archive");

        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;
        config.PartSizeLimit = 3 * (1 + sizeof(ui32));

        {
            auto archive = TEngine::Create(arcPath, config, 0);

            for (ui32 docId = 0; docId < 10; ++docId) {
                archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
            }
            archive->RemoveDocument(3);
            archive->RemoveDocument(4);
            archive->RemoveDocument(5);
            archive->WaitAllTasks();

            UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 3);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 7);
            archive.Drop();

            TFsPath fatFile(arcPath.GetPath() + ".fat");
            fatFile.CopyTo(arcPath.GetPath() + ".tmp", true);

            TEngine::Remove(arcPath);

            archive = TEngine::Create(arcPath, config, 0);

            for (ui32 docId = 0; docId < 10; ++docId) {
                archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
            }

            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 10);
            archive.Drop();
            fatFile = TFsPath(arcPath.GetPath() + ".tmp");
            fatFile.CopyTo(arcPath.GetPath() + ".fat", true);
            NFs::Remove(fatFile);
        }

        {
            GetPartMetaPath(arcPath, 1).ForceDelete();
            auto archive = TEngine::Create(arcPath, config, 0, true);

            UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 4);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 7);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(true), 10);

            archive.Drop();
            GetPartMetaPath(arcPath, 1).ForceDelete();
            UNIT_ASSERT(!TEngine::Check(arcPath));
            TEngine::Repair(arcPath, config);
            UNIT_ASSERT(TEngine::Check(arcPath));

            archive = TEngine::Create(arcPath, config, 0, true);

            UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 3);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 7);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(true), 10);
        }
    }

    Y_UNIT_TEST(TestBrokenPart) {
        TFsPath arcPath("test_archive");

        TMultipartConfig config;
        config.WriteContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.ReadContextDataAccessType = IDataAccessor::FILE;
        config.PartSizeLimit = 3 * (1 + sizeof(ui32));

        auto archive = TEngine::Create(arcPath, config, 0);

        TVector<ui32> checkDocs;
        for (ui32 docId = 0; docId < 9; ++docId) {
            auto pos = archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
            if (pos.GetPart() == 2) {
                checkDocs.push_back(docId);
            }
        }

        TFsPath fatFile(arcPath.GetPath() + ".fat");
        fatFile.CopyTo(arcPath.GetPath() + ".tmp", true);

        archive.Drop();
        TEngine::Remove(arcPath);
        archive = TEngine::Create(arcPath, config, 0);

        for (ui32 docId = 0; docId < 7; ++docId) {
            archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
        }

        archive.Drop();
        fatFile = TFsPath(arcPath.GetPath() + ".tmp");
        fatFile.CopyTo(arcPath.GetPath() + ".fat", true);

        GetPartMetaPath(arcPath, 2).ForceDelete();

        UNIT_ASSERT(!TEngine::Check(arcPath));
        TEngine::Repair(arcPath, config);
        UNIT_ASSERT(TEngine::Check(arcPath));

        {
            auto archive = TEngine::Create(arcPath, config, 0);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 4);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 7);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(true), 9);
            UNIT_ASSERT_VALUES_EQUAL(checkDocs.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(archive->IsRemoved(checkDocs.front()), false);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocument(checkDocs.back()).Empty(), true);
            UNIT_ASSERT_VALUES_EQUAL(archive->IsRemoved(checkDocs.back()), true);
        }
        NFs::Remove(fatFile);
    }
}

Y_UNIT_TEST_SUITE_IMPL(ArchiveSuite, TMultipartArchiveTestBase) {
    void TestArchive(IArchivePart::TType partType, IDataAccessor::TType readAccType) {
        TFsPath arcPath("my_archive");
        TEngine::Remove(arcPath);

        TMultipartConfig config;
        config.ReadContextDataAccessType = readAccType;
        config.Compression = partType;

        TSet<ui32> allDocids;
        for (ui32 i = 0; i < 3; ++i) {
            auto archive = TEngine::Create(arcPath, config, 20);
            for (ui32 j = 0; j < 3; ++j) {
                ui32 docid = (3 * i + j) * 2 + 1;
                allDocids.insert(docid);
                archive->PutDocument(TBlob::FromString(ToString(docid)), docid);
            }
        }
        auto archiveR = TEngine::Create(arcPath, config);
        for (const auto& i : allDocids) {
            TBlob doc = archiveR->GetDocument(i);
            UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc.AsCharPtr(), doc.Size()), ToString(i));
        }
        TSet<ui32> docids;
        for (auto i = archiveR->CreateIterator(); i->IsValid(); i->Next()) {
            TBlob doc = i->GetDocument();
            UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc.AsCharPtr(), doc.Size()), ToString(i->GetDocid()));
            docids.insert(i->GetDocid());
        }
        UNIT_ASSERT_VALUES_EQUAL(docids.size(), allDocids.size());
        for (const auto& i : docids)
            UNIT_ASSERT(allDocids.contains(i));
        archiveR.Drop();
        auto archive = TEngine::Create(arcPath, config, 10);
        for (const auto& i : docids)
            archive->RemoveDocument(i);
        TVector<TFsPath> children;
        archive.Drop();
        INFO_LOG << "Drop arc" << Endl;
        TFsPath(".").List(children);
        for (auto& f : children) {
            UNIT_ASSERT(!f.GetName().StartsWith("my_archive.part."));
        }
    }

    void TestArchiveGarps(IArchivePart::TType partType, IDataAccessor::TType readAccType) {
        TFsPath arcPath("my_archive");
        TEngine::Remove(arcPath);

        TMultipartConfig config;
        config.ReadContextDataAccessType = readAccType;
        config.Compression = partType;

        auto archive = TEngine::Create(arcPath, config, 2000);
        {
            ui32 docid = 0;
            archive->PutDocument(TBlob::FromString(ToString(docid)), docid);
            archive->RemoveDocument(docid);
        }
        TSet<ui32> docids;
        for (auto i = archive->CreateIterator(); i->IsValid(); i->Next()) {
            TBlob doc = i->GetDocument();
            UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc.AsCharPtr(), doc.Size()), ToString(i->GetDocid()));
            docids.insert(i->GetDocid());
        }
        UNIT_ASSERT(docids.empty());
        archive.Drop();
        TVector<TFsPath> children;
        TFsPath(".").List(children);
        for (auto& f : children)
            UNIT_ASSERT(!f.GetName().StartsWith("my_archive.part."));
    }

    Y_UNIT_TEST(TestMultipleParts) {
        TFsPath arcPath("my_archive");

        TMultipartConfig config;
        config.ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.PartSizeLimit = 3 * (1 + sizeof(ui32));

        auto archive = TEngine::Create(arcPath, config, 10);

        for (ui32 docId = 0; docId < 10; ++docId) {
            archive->PutDocument(TBlob::FromString(ToString(docId)), docId);
        }

        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 4);
        archive->RemoveDocument(3);
        archive->RemoveDocument(4);
        archive->RemoveDocument(5);
        UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 3);
    }

    Y_UNIT_TEST(TestDifferentCompressions) {
        TFsPath arcPath("test_archive");

        TMultipartConfig config;
        config.ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.Compression = IArchivePart::RAW;
        config.PartSizeLimit = 3 * (1001 + sizeof(ui32));

        {
            INFO_LOG << "Adding 10 documents in raw parts..." << Endl;
            auto archive = TEngine::Create(arcPath, config, 10);

            for (ui32 docId = 0; docId < 10; ++docId) {
                archive->PutDocument(TBlob::FromString(TString(1000, docId)), docId);
            }
            INFO_LOG << "Adding 10 documents in raw parts... OK" << Endl;
        }
        config.Compression = IArchivePart::COMPRESSED_EXT;
        {
            INFO_LOG << "Adding 10 documents in compressedExt parts..." << Endl;
            auto archive = TEngine::Create(arcPath, config);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 4);
            for (ui32 docId = 10; docId < 20; ++docId) {
                archive->PutDocument(TBlob::FromString(TString(1000, docId)), docId);
            }
            INFO_LOG << "Adding 10 documents in compressedExt parts... OK" << Endl;
            INFO_LOG << "Checking documents data..." << Endl;
            TSet<ui32> docids;
            for (auto i = archive->CreateIterator(); i->IsValid(); i->Next()) {
                TBlob doc = i->GetDocument();
                UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc.AsCharPtr(), doc.Size()), TString(1000, i->GetDocid()));
                docids.insert(i->GetDocid());
            }
            UNIT_ASSERT_VALUES_EQUAL(docids.size(), 20);
            INFO_LOG << "Checking documents data... OK" << Endl;
        }
        config.Compression = IArchivePart::RAW;
        {
            INFO_LOG << "Removing every even docid..." << Endl;
            auto archive = TEngine::Create(arcPath, config);
            for (ui32 docId = 0; docId < 20; docId += 2) {
                archive->RemoveDocument(docId);
            }
            INFO_LOG << "Removing every even docid... OK" << Endl;
            INFO_LOG << "Merging parts..." << Endl;
            NRTYArchive::TOptimizationOptions optimizationOptions;
            optimizationOptions.SetPopulationRate(0.9);
            archive->OptimizeParts(optimizationOptions);
            INFO_LOG << "Merging parts... OK" << Endl;
        }

        {
            INFO_LOG << "Checking documents data..." << Endl;
            auto archive = TEngine::Create(arcPath, config);
            TSet<ui32> docids;
            for (auto i = archive->CreateIterator(); i->IsValid(); i->Next()) {
                TBlob doc = i->GetDocument();
                UNIT_ASSERT_VALUES_EQUAL(TStringBuf(doc.AsCharPtr(), doc.Size()), TString(1000, i->GetDocid()));
                docids.insert(i->GetDocid());
            }
            UNIT_ASSERT_VALUES_EQUAL(docids.size(), 10);
            INFO_LOG << "Checking documents data... OK" << Endl;
        }
        {
            INFO_LOG << "Removing all docids except docid 1..." << Endl;
            auto archive = TEngine::Create(arcPath, config);
            for (ui32 docId = 2; docId < 20; docId++) {
                archive->RemoveDocument(docId);
            }
            INFO_LOG << "Removing all docids except docid 1... OK" << Endl;
            INFO_LOG << "Merging parts..." << Endl;
            NRTYArchive::TOptimizationOptions optimizationOptions;
            optimizationOptions.SetPopulationRate(0.9);
            archive->OptimizeParts(optimizationOptions);
            INFO_LOG << "Merging parts... OK" << Endl;
        }
        {
            INFO_LOG << "Checking documents data..." << Endl;
            auto archive = TEngine::Create(arcPath, config);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetDocsCount(false), 1);
            UNIT_ASSERT_VALUES_EQUAL(archive->GetPartsCount(), 2); // Part with document and new open empty part"
            INFO_LOG << "Checking documents data... OK" << Endl;
        }
    }

    Y_UNIT_TEST(TestPutGet) {
        IDataAccessor::TType readTypes[] = {
            IDataAccessor::DIRECT_FILE,
            IDataAccessor::FILE,
            IDataAccessor::MEMORY_MAP,
            IDataAccessor::MEMORY_LOCKED_MAP,
            IDataAccessor::MEMORY_PRECHARGED_MAP,
            IDataAccessor::MEMORY_RANDOM_ACCESS_MAP,
            IDataAccessor::MEMORY_FROM_FILE
        };
        IArchivePart::TType partTypes[] = { IArchivePart::RAW, IArchivePart::COMPRESSED };
        for (ui32 p = 0; p < Y_ARRAY_SIZE(partTypes); ++p)
            for (ui32 rt = 0; rt < Y_ARRAY_SIZE(readTypes); ++rt) {
                TestArchive(partTypes[p], readTypes[rt]);
                TestArchiveGarps(partTypes[p], readTypes[rt]);
            }
    }

    Y_UNIT_TEST(TestCompressed) {
        TFsPath arcPath("my_archive");

        TMultipartConfig config;
        config.ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.Compression = IArchivePart::COMPRESSED;

        TVector<TString> docs;
        ui32 size = 0;
        while (size < 1 << 16) {
            docs.push_back(TString(10000, 'A' + docs.size() % 26));
            size += docs.back().size();
        }
        auto archive = TEngine::Create(arcPath, config, docs.size());
        for (ui32 i = 0; i < docs.size(); ++i) {
            archive->PutDocument(TBlob::NoCopy(docs[i].data(), docs[i].size()), i);
        }
        for (ui32 i = 0; i < docs.size(); ++i) {
            TBlob doc = archive->GetDocument(i);
            UNIT_ASSERT_VALUES_EQUAL(doc.Size(), docs[i].size());
            UNIT_ASSERT_VALUES_EQUAL(memcmp(doc.AsCharPtr(), docs[i].data(), doc.Size()), 0);
        }
        TSet<ui32> docids;
        for (auto i = archive->CreateIterator(); i->IsValid(); i->Next()) {
            TBlob doc = i->GetDocument();
            ui32 docid = i->GetDocid();
            UNIT_ASSERT_VALUES_EQUAL(doc.Size(), docs[docid].size());
            UNIT_ASSERT_VALUES_EQUAL(memcmp(doc.AsCharPtr(), docs[docid].data(), doc.Size()), 0);
            docids.insert(i->GetDocid());
        }
        UNIT_ASSERT_VALUES_EQUAL(docids.size(), docs.size());
        UNIT_ASSERT_VALUES_EQUAL(*docids.rbegin(), docs.size() - 1);
    }

    Y_UNIT_TEST(TestCompressedSize) {
        TMultipartConfig configC;
        configC.ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        configC.Compression = IArchivePart::COMPRESSED;

        TMultipartConfig configR;
        configR.ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        configR.Compression = IArchivePart::RAW;

        TVector<TString> docs;
        docs.reserve(1000);
        for (ui32 i = 0; i < 1000; ++i) {
            TString word;
            for (ui32 p = 0; p < 100; ++p)
                word.append('A' + (37 * i + 23 * p) % 60);
            docs.push_back(word);
        }
        auto archiveR = TEngine::Create("archive_raw", configR, docs.size());
        auto archiveC = TEngine::Create("archive_compressed", configC, docs.size());
        for (ui32 i = 0; i < docs.size(); ++i) {
            archiveR->PutDocument(TBlob::NoCopy(docs[i].data(), docs[i].size()), i);
            archiveC->PutDocument(TBlob::NoCopy(docs[i].data(), docs[i].size()), i);
        }
        archiveR.Drop();
        archiveC.Drop();
        i64 rawSize = TFile("archive_raw.part.0", RdOnly).GetLength();
        i64 compressedSize = TFile("archive_compressed.part.0", RdOnly).GetLength();
        UNIT_ASSERT(rawSize > 3 * compressedSize);
    }

    Y_UNIT_TEST(TestPreallocation) {
        TMultipartConfig config;
        config.ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.PartSizeLimit = 1_GB;
        config.PreallocateParts = true;

        TVector<TString> docs(100, TString(100, 'A'));
        auto archive = TEngine::Create("archive", config, docs.size());
        for (ui32 i = 0; i < docs.size(); ++i) {
            archive->PutDocument(TBlob::NoCopy(docs[i].data(), docs[i].size()), i);
        }
        archive.Drop();
        TFileStat stat(TFile("archive.part.0", RdOnly));
        UNIT_ASSERT(stat.Size == 100 * (100 + sizeof(ui32)));
        UNIT_ASSERT(stat.AllocationSize <= stat.Size + 2 * NSystemInfo::GetPageSize());
    }

    Y_UNIT_TEST(TestSequense) {
        TVector<TString> docs;
        ui32 size = 0;
        while (size < 1 << 16) {
            docs.push_back(TString(10000, 'A' + docs.size() % 26));
            size += docs.back().size();
        }

        TMultipartConfig config;
        config.ReadContextDataAccessType = IDataAccessor::DIRECT_FILE;
        config.Compression = IArchivePart::COMPRESSED;

        auto archive = TEngine::Create("src", config, docs.size());
        for (ui32 i = 0; i < docs.size(); ++i)
            archive->PutDocument(TBlob::NoCopy(docs[i].data(), docs[i].size()), i);
        TVector<ui32> docids;
        for (auto i = archive->CreateIterator(); i->IsValid(); i->Next()) {
            TBlob doc = i->GetDocument();
            ui32 docid = i->GetDocid();
            UNIT_ASSERT_VALUES_EQUAL(doc.Size(), docs[docid].size());
            UNIT_ASSERT_VALUES_EQUAL(memcmp(doc.AsCharPtr(), docs[docid].data(), doc.Size()), 0);
            docids.push_back(docid);
        }
        UNIT_ASSERT_VALUES_EQUAL(docids.size(), docs.size());
        TVector<ui32>::const_iterator docidIter = docids.begin();
        for (auto i = archive->CreateIterator(); i->IsValid(); i->Next()) {
            TBlob doc = i->GetDocument();
            ui32 docid = i->GetDocid();
            UNIT_ASSERT_VALUES_EQUAL(doc.Size(), docs[docid].size());
            UNIT_ASSERT_VALUES_EQUAL(memcmp(doc.AsCharPtr(), docs[docid].data(), doc.Size()), 0);
            UNIT_ASSERT_VALUES_EQUAL(docid, *docidIter);
            ++docidIter;
        }
    }
}
