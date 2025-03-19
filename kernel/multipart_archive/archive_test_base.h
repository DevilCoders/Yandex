#pragma once

#include "owner.h"
#include "multipart.h"
#include "test_utils.h"

#define ARCHIVE_PREFIX "test_arc_"

using TEngine = TArchiveOwner;

class TMultipartArchiveTestBase : public TTestBase {
public:
    virtual ELogPriority GetLogLevel() const {
        return TLOG_DEBUG;
    }

    virtual void SetUp() {
        DoInitGlobalLog("console", GetLogLevel(), false, false);

        DEBUG_LOG << "Set up " << Name() << Endl;
        Clear();
    }

    virtual void TearDown() {
        DEBUG_LOG << "Tear down " << Name() << Endl;
        Clear();
    }

public:
    static void Clear() {
        using namespace NRTYArchive;

        TVector<TFsPath> files;
        TFsPath::Cwd().List(files);

        TVector<TFsPath> results;

        for (const auto& child : files) {
            const TString file(child.GetName());
            if ("." + child.GetExtension() == FAT_EXT) {
                results.push_back(file.substr(0, file.size() - FAT_EXT.size()));
            }
        }

        for (auto&& archive : results) {
            TArchiveOwner::Remove(archive);
        }

        files.clear();
        TFsPath::Cwd().List(files);
        for (const auto& child : files) {
            if (child.GetName().StartsWith(ARCHIVE_PREFIX)) {
                child.DeleteIfExists();
            }
        }
    }
};

class TPartCallBack: public NRTYArchive::IPartCallBack {
public:
    virtual void OnPartFree(ui64 /*partNum*/) override {}
    virtual void OnPartFull(ui64 /*partNum*/) override {}
    virtual void OnPartDrop(ui64 /*partNum*/) override {}
    virtual void OnPartClose(ui64 /*partNum*/, ui64 /*docsCount*/) override {}
};



namespace NRTYArchive {
    class TArchiveConstructor {
    public:
        static void GenerateData(ui64 docsCount, NRTYArchiveTest::TArchiveDataGen::TDataSet& data, ui64 docSize = 0) {
            NRTYArchiveTest::TArchiveDataGen generator(docSize);
            generator.Generate(docsCount, data);
            CHECK_WITH_LOG(data.size() == docsCount);
        }

        static TEngine::TPtr ConstructDirect(const NRTYArchiveTest::TArchiveDataGen::TDataSet& data, ui32 partsCount = 0, const TFsPath& path = "") {
            return ConstructImpl(data, partsCount, path, IDataAccessor::DIRECT_FILE, IArchivePart::RAW);
        }

        static TEngine::TPtr ConstructCompressedDirect(const NRTYArchiveTest::TArchiveDataGen::TDataSet& data, ui32 partsCount = 0, const TFsPath& path = "") {
            return ConstructImpl(data, partsCount, path, IDataAccessor::DIRECT_FILE, IArchivePart::COMPRESSED);
        }

        static TEngine::TPtr ConstructCompressedMem(const NRTYArchiveTest::TArchiveDataGen::TDataSet& data, ui32 partsCount = 0, const TFsPath& path = "") {
            return ConstructImpl(data, partsCount, path, IDataAccessor::MEMORY_FROM_FILE, IArchivePart::COMPRESSED);
        }

        static TEngine::TPtr GetCompressedMem(const TFsPath& path, ui64 sizeLimit) {
            TMultipartConfig config;
            config.ReadContextDataAccessType = IDataAccessor::MEMORY_FROM_FILE;
            config.Compression = IArchivePart::COMPRESSED;
            config.PartSizeLimit = sizeLimit;
            return TEngine::Create(path, config);
        }

    private:
        static TEngine::TPtr ConstructImpl(const NRTYArchiveTest::TArchiveDataGen::TDataSet& data, ui32 partsCount, const TFsPath& path, IDataAccessor::TType accessorType, IArchivePart::TType partType) {
            bool useOnePart = (partsCount == 0);
            ui64 docsCount = data.size();


            ui64 docsPerPart = data.size();
            if (!useOnePart) {
                ui64 tail = docsCount % partsCount;
                docsPerPart = tail ? (docsCount) / (partsCount - 1) : docsCount / partsCount;
            }

            CHECK_WITH_LOG(data.size() == docsCount);

            TMultipartConfig config;
            config.ReadContextDataAccessType = accessorType;
            config.Compression = partType;

            auto archive = TEngine::Create(path ? path : DefaultPath, config, docsCount);

            ui64 index = 0;
            ui64 sizeInBytes = 0;
            ui64 maxSizeInBytes = 0;
            for (auto&& doc : data) {
                if (index == docsPerPart) {
                    archive->Flush();
                    archive->WaitAllTasks();
                    ui64 bytes = archive->GetSizeInBytes();
                    maxSizeInBytes = Max<ui64>(bytes - sizeInBytes, maxSizeInBytes);
                    sizeInBytes = bytes;
                    index = 0;
                }

                archive->PutDocument(doc.Value, doc.Key);
                index++;
            }

            CHECK_WITH_LOG(archive->GetDocsCount(true) == docsCount);

            if (!useOnePart) {
                archive.Drop();
                config.PartSizeLimit = maxSizeInBytes;
                archive = TEngine::Create(path ? path : DefaultPath, config, 0);
            } else {
                archive->Flush();
            }

            INFO_LOG << "Generate: docsCount=" << docsPerPart << ";size_limit=" << maxSizeInBytes << ";parts_count=" << archive->GetPartsCount() << Endl;

            if (partsCount != 0) {
                CHECK_WITH_LOG(archive->GetPartsCount() == partsCount + 1) << partsCount << "/" << archive->GetPartsCount();
            }

            return archive;
        }

    private:
        static const TFsPath DefaultPath;
    };
}
