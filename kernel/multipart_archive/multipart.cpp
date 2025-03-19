#include "multipart.h"
#include "archive_fat.h"
#include <kernel/multipart_archive/abstract/globals.h>
#include <kernel/multipart_archive/iterators/factory.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/algorithm.h>
#include <util/system/filemap.h>
#include <util/system/fs.h>
#include <util/digest/fnv.h>

#include <kernel/multipart_archive/protos/archive.pb.h>

namespace {
    struct TPartState : NRTYArchive::IPartState {
        ui64 DocsCount = 0;
        ui64 SizeInBytes = 0;
        ui64 HeaderSize = 0;
        ui64 PartSizeLimit = 0;
        bool Writable = false;

        ui64 GetDocsCount(bool withDeleted) const final {
            return withDeleted ? HeaderSize : DocsCount;
        }

        ui64 GetSizeInBytes() const final {
            return SizeInBytes;
        }

        ui64 GetPartSizeLimit() const final {
            return PartSizeLimit;
        }

        TFsPath GetPath() const final {
            return "";
        }
    };
}

namespace NRTYArchive {
    TMultipartArchive::TMultipartArchive(const TFsPath& path, const IArchivePart::TConstructContext& ctx, const TFsPath& offsetsPath)
        : TMultipartImpl(path, nullptr, new TFakeArchiveManager(ctx), nullptr)
    {
        InitFromOffsetFile(offsetsPath);
    }

    TMultipartArchive::TMultipartArchive(const TFsPath& path, IFat* fat, IArchiveManager* manager, const NNamedLock::TNamedLockPtr& archiveLock)
        : TMultipartImpl(path, fat, manager, archiveLock)
    {
        InitCommon();
    }

    void TMultipartArchive::Remove(const TFsPath& path) {
        TVector<ui32> partIndexes;
        TMultipartImpl::FillPartsIndexes(path, partIndexes);
        for (ui64 index : partIndexes) {
            TArchivePartThreadSafe::Remove(path, index);
        }
        if (GetFatPath(path).Exists()) {
            CHECK_WITH_LOG(NFs::Remove(GetFatPath(path)));
        }
    }


    void TMultipartArchive::Rename(const TFsPath& from, const TFsPath& to) {
        TVector<ui32> partIndexes;
        TMultipartImpl::FillPartsIndexes(from, partIndexes);
        for (ui64 index : partIndexes) {
            TArchivePartThreadSafe::Rename(from, to, index);
        }
        CHECK_WITH_LOG(GetFatPath(from).Exists());
        GetFatPath(from).RenameTo(GetFatPath(to));
    }

    bool TMultipartArchive::AllRight(const TFsPath& path) {
        TVector<TFsPath> files;
        path.Parent().List(files);
        for (const auto& file : files) {
            ui32 index = TArchivePartThreadSafe::ParsePartFromFileName(file, path);
            if (index != Max<ui32>()) {
                if (!GetPartHeaderPath(path, index).Exists()  || !TArchivePartThreadSafe::AllRight(path, index)) {
                    WARNING_LOG << "Broken part " << file.GetPath() << Endl;
                    return false;
                }
            }
        }
        return true;
    }


    bool TMultipartArchive::IsEmpty(const TFsPath& path) {
        try {
            TFATMultipart fat(GetFatPath(path), 0, TMemoryMapCommon::oRdOnly);
            for (ui32 i = 0; i < fat.Size(); ++i)
                if (!fat.Get(i).IsRemoved())
                    return false;
        } catch (...) {
            ERROR_LOG << "Exception in TMultipartArchiveEngine::Empty: " << CurrentExceptionMessage() << Endl;
        }
        return true;
    }

    NRTYArchive::TMultipartArchive* TMultipartArchive::Construct(const TFsPath& path, const TMultipartConfig& config, const NNamedLock::TNamedLockPtr& archiveLock, ui32 docsCount /*= 0*/, bool readonly /*= false*/) {
        TMemoryMapCommon::EOpenMode fatMode = (readonly ? TMemoryMapCommon::oRdOnly : TMemoryMapCommon::oRdWr);
        if (config.PrechargeFAT) {
            fatMode |= TMemoryMapCommon::oPrecharge;
        }

        TAutoPtr<TFATMultipart> fat = new TFATMultipart(GetFatPath(path), docsCount, fatMode);
        return new TMultipartArchive(path, fat.Release(), new TArchiveManager(config.CreateReadContext(), readonly, config.WritableThreadsCount), archiveLock);
    }

    class TRepairer: public TArchivePartThreadSafe::IRepairer {
    public:
        using THeaderInfo = TMap<ui64, ui32>;

        TRepairer(const THeaderInfo& headerInfo, TFATMultipart& fat)
            : HeaderInfo(headerInfo)
            , FAT(fat)
            , Current(HeaderInfo.begin())
        {}

        void OnIterNext(ui32, ui64 offset) override {
            if (Current == HeaderInfo.end()) {
                Result.push_back(static_cast<ui32>(IPartHeader::EFlags::DOC_REMOVED));
                return;
            }

            if (offset == Current->first) {
                Result.push_back(Current->second);
                Current++;
                RepairedCount++;
            } else {
                Result.push_back(static_cast<ui32>(IPartHeader::EFlags::DOC_REMOVED));
            }
        }

        const TVector<ui32>& GetHeader() const {
            return Result;
        }

        ui32 GetFullDocsCount() const override {
            return GetHeader().size();
        }

        ui32 GetRepairedDocsCount() const override {
            return RepairedCount;
        }

        ui32 GetLost() const override {
            return Lost;
        }

        void Flush(const TFsPath& path) override {
            while (Current != HeaderInfo.end()) {
                FAT.Set(Current->second, TPosition());
                Lost++;
                Current++;
            }
            IPartHeader::SaveRestoredHeader(path, GetHeader());
        }
    private:
        const THeaderInfo& HeaderInfo;
        TFATMultipart& FAT;
        THeaderInfo::const_iterator Current;
        TVector<ui32> Result;
        ui32 RepairedCount = 0;
        ui32 Lost = 0;
    };

    void TMultipartArchive::Repair(const TFsPath& path, const IArchivePart::TConstructContext& ctx) {
        CHECK_WITH_LOG(GetFatPath(path).Exists());

        ui32 initialFatSize;
        {
            TFATMultipart roFat(GetFatPath(path), 0, TMemoryMapCommon::oRdOnly);
            initialFatSize = roFat.Size();
        }
        TFATMultipart fat(GetFatPath(path), 0, TMemoryMapCommon::oRdWr);
        CHECK_WITH_LOG(initialFatSize == fat.Size()) << fat.Size() << "/" << initialFatSize;

        TVector<ui32> partIndexes;
        TMultipartImpl::FillPartsIndexes(path, partIndexes);

        TMap<ui64, TRepairer::THeaderInfo> docsOffsetsPerPart;

        for (auto fatIter = fat.GetIterator(); fatIter->IsValid(); fatIter->Next()) {
            const TPosition pos = fatIter->GetPosition();
            if (pos.IsRemoved())
                continue;

            docsOffsetsPerPart[pos.GetPart()].insert(std::make_pair(pos.GetOffset(), fatIter->GetId()));
        }

        for (ui32 i = 0; i < partIndexes.size(); ++i) {
            TFsPath headerPath = GetPartHeaderPath(path, partIndexes[i]);
            if (!headerPath.Exists() || !TArchivePartThreadSafe::AllRight(path, partIndexes[i])) {

                auto iter = docsOffsetsPerPart.find(partIndexes[i]);

                if (iter == docsOffsetsPerPart.end() || iter->second.size() == 0) {
                    WARNING_LOG << "Remove empty part " << headerPath << Endl;
                    TArchivePartThreadSafe::Remove(path, partIndexes[i]);
                    continue;
                }

                const TRepairer::THeaderInfo& offset2Docid = iter->second;
                const TPartMetaSaver partMeta(GetPartMetaPath(path, partIndexes[i]), /*forceOpen*/ false, /*allowUnknownFields*/ true);

                if (headerPath.Exists() && partMeta->GetStage() == TPartMetaInfo::OPENED) {
                    WARNING_LOG << "Repair opened part " << headerPath << Endl;
                    TFileMappedArray<ui32> header;
                    header.Init(headerPath.c_str());
                    TPartMetaSaver mutableMeta(GetPartMetaPath(path, partIndexes[i]), /*forceOpen*/ false, /*allowUnknownFields*/ true);
                    mutableMeta->SetDocsCount(header.size());
                    mutableMeta->SetRemovedDocsCount(header.size() - offset2Docid.size());
                    mutableMeta->SetStage(TPartMetaInfo::CLOSED);
                } else {
                    TRepairer repairer(offset2Docid, fat);
                    if (!TArchivePartThreadSafe::Repair(path, partIndexes[i], ctx, &repairer)) {
                        WARNING_LOG << "Can't restore part " << headerPath << Endl;
                        TArchivePartThreadSafe::Remove(path, partIndexes[i]);
                    }
                }
            }
        }
        CHECK_WITH_LOG(initialFatSize == fat.Size()) << fat.Size() << "/" << initialFatSize;
    }

    void TMultipartArchive::InitFromOffsetFile(const TFsPath& offsetsPath) {
        VERIFY_WITH_LOG(Path.Exists(), "part file %s does not exist", Path.GetPath().data());
        VERIFY_WITH_LOG(offsetsPath.Exists(), "offsets file %s does not exist", offsetsPath.GetPath().data());

        FAT.Reset(new TFATBaseArchive(offsetsPath));
        THolder<TArchivePartThreadSafe> part = MakeHolder<TArchivePartThreadSafe>(Path, Max<ui32>(), Manager.Get(), false, *this);

        CHECK_WITH_LOG(!Parts.contains(0));
        CHECK_WITH_LOG(Parts.insert(std::make_pair(0, part.Release())).second);
    }

    TMultipartArchive::TIteratorPtr TMultipartArchive::CreateIterator(bool raw) {
        if (raw) {
            return CreateCommonIterator(new TRawIteratorFactory);
        }
        return CreateCommonIterator(new TOffsetsIteratorFactory(FAT.Get()));
    }

    TVector<size_t> TMultipartArchive::GetDocIds() {
        return GetIdsFromSnapshot(FAT->GetSnapshot());
    }

    THashMap<ui32, TPosition> TMultipartArchive::GetDocumentsInfo() const {
        THashMap<ui32, TPosition> documetsInfo;

        for (auto fatIter = FAT->GetIterator(); fatIter->IsValid(); fatIter->Next()) {
            const TPosition pos = fatIter->GetPosition();
            if (pos.IsRemoved())
                continue;
            documetsInfo[fatIter->GetId()] = pos;
        }

        return documetsInfo;
    }

    TArchivePartThreadSafe::TPtr TMultipartArchive::HardLinkOrCopyPart(ui64 partNum, const TFsPath& newPath, ui64 newPartNum, IArchiveManager* manager, ui32 newDocsCount, IPartCallBack& owner) const {
        TPartPtr part = GetPartByNum(partNum);
        if (!part) {
            DEBUG_LOG << "No part " << partNum << " in " << Path << Endl;
            return nullptr;
        }
        if (part->IsWritable()) {
            CHECK_WITH_LOG(part->GetDocsCount(true) == 0);
            return nullptr;
        }

        return part->HardLinkOrCopyTo(newPath, newPartNum, manager, newDocsCount, owner);
    }

    void TMultipartArchive::Append(const TMultipartArchive& other, const TRemap* remap, bool) {
        CHECK_WITH_LOG(IsWritable());
        THashMap<ui32, TPosition> documetsInfo = other.GetDocumentsInfo();

        TRemap old2NewIds;
        if (remap) {
            for (auto&& rule : *remap) {
                old2NewIds[rule.second] = rule.first;
            }
        } else {
            for (auto&& info : documetsInfo) {
                old2NewIds[info.first] = info.first;
            }
        }

        TMap<ui64, ui32> partsToCopy;
        for (auto&& doc : old2NewIds) {
            auto it = documetsInfo.find(doc.first);
            if (it != documetsInfo.end()) {
                partsToCopy[it->second.GetPart()]++;
            }
        }

        TMap<ui64, ui64> newPartNums;
        {
            TWriteGuard g(Lock);
            for (const auto& p : partsToCopy) {
                DEBUG_LOG << "Allocate new part for " << other.GetPath() << "/" << p.first << "(" << p.second << ") -> " << Path << "/" << FreePartIndex << Endl;
                TArchivePartThreadSafe::TPtr newPart = other.HardLinkOrCopyPart(p.first, Path, FreePartIndex, Manager.Get(), p.second, *this);
                if (newPart) {
                    newPartNums[p.first] = FreePartIndex;
                    DEBUG_LOG << "Copy " << other.GetPath() << "/" << p.first << "(" << p.second << ") -> " << Path << "/" << FreePartIndex << Endl;
                    Parts[FreePartIndex] = newPart;
                    ++FreePartIndex;
                }
            }
        }

        for (auto&& remapDocRule : old2NewIds) {
            ui32 newId = remapDocRule.second;

            auto it = documetsInfo.find(remapDocRule.first);
            if (it == documetsInfo.end()) {
                continue;
            }

            ui64 oldPartNum = it->second.GetPart();

            auto remapPartRule = newPartNums.find(oldPartNum);
            if (remapPartRule == newPartNums.end())
                continue;

            if (other.IsRemoved(remapDocRule.first)) {
                RemoveDocumentFromPart(remapPartRule->second, [](const TArchivePartThreadSafe::TPtr&){});
                continue;
            }

            CHECK_WITH_LOG(GetPartByNum(remapPartRule->second)->GetDocsCount(false) != 0) << Path << "/" << remapPartRule->second;

            TPosition newPos = it->second;
            newPos.SetPart(remapPartRule->second);
            FAT->Set(newId, newPos);
        }
    }

    void TMultipartArchive::Remap(const TVector<ui32>& remap) {
        TVector<TPosition> newFat;
        newFat.reserve(FAT->Size());
        for (ui32 docid = 0; docid < remap.size(); ++docid) {
            ui32 newDocid = remap[docid];
            TPosition pos = FAT->Get(docid);
            if (newDocid != Max<ui32>()) {
                if (newFat.size() <= newDocid)
                    newFat.resize(newDocid + 1, TPosition::Removed());
                newFat[newDocid] = pos;
            }
        }
        TFsPath tmpFatPath(GetFatPath(Path).GetPath() + ".tmp");
        tmpFatPath.ForceDelete();
        TAutoPtr<TFATMultipart> diskFat(new TFATMultipart(tmpFatPath, newFat.size(), TMemoryMapCommon::oRdWr));
        for (ui32 i = 0; i < newFat.size(); ++i) {
            diskFat->Set(i, newFat[i]);
        }

        FAT.Reset(diskFat.Release());
        tmpFatPath.ForceRenameTo(GetFatPath(Path));
    }

    bool TMultipartArchive::OptimizeParts(const NRTYArchive::TOptimizationOptions& options, const std::atomic<bool>* stopSignal) {
        INFO_LOG << Path.GetPath() << ": Optimize parts (" << options.GetPopulationRate() << "/" << options.GetPartSizeDeviation() << ")..." << Endl;
        NRTYArchive::TPartOptimizationCheckVisitor partOptimizationCheckVisitor(options);
        {
            TWriteGuard wg(Lock);

            partOptimizationCheckVisitor.Start();
            for (auto& part : Parts) {
                if (part.second->IsWritable()) {
                    continue;
                }
                partOptimizationCheckVisitor.VisitPart(part.second->GetPartNum(), *part.second.Get());
            }
        }

        const TSet<ui32> partsToRemove = partOptimizationCheckVisitor.GetOptimizablePartNums();

        if (partsToRemove.empty()) {
            INFO_LOG << Path.GetPath() << ": Optimize parts ...NOTHING" << Endl;
            return true;
        }

        INFO_LOG << Path.GetPath() << ": " << partsToRemove.size() << " parts to optimize" << Endl;

        for (ui32 partIndex : partsToRemove) {
            TArchivePartThreadSafe::TPtr part = GetPartByNum(partIndex);
            if (!part)
                continue;

            ui32 singleDocs = 0;
            INFO_LOG << "Free part: " << part->GetPath() << "..." << Endl;
            TOffsetsIteratorFactory factory(FAT.Get());
            for (IPartIterator::TPtr iter = factory.CreatePartIterator(part, FAT.Get()); iter->IsValid(); iter->Next()) {
                if (stopSignal && *stopSignal)
                    return false;
                TBlob document = iter->GetDocument();
                if (document.Empty()) {
                    ERROR_LOG << "Remove not flushed doc " << iter->GetDocId() << Endl;
                    RemoveDocument(iter->GetDocId());
                    continue;
                }
                PutDocument(document, iter->GetDocId(), 1);
                ++singleDocs;
            }
            INFO_LOG << "Free part: " << part->GetPath() << "...OK (" << singleDocs << " single docs)" << Endl;
        }

        INFO_LOG << Path.GetPath() << ": Optimize parts (" << options.GetPopulationRate() << "/" << options.GetPartSizeDeviation() << ")...OK" << Endl;
        return true;
    }

    static void FillOptimization(float population, const TVector<TPartState>& partsInfos, TArchiveInfo& info) {
        auto options = TOptimizationOptions().SetPopulationRate(population);
        NRTYArchive::TPartOptimizationCheckVisitor partOptimizationCheckVisitor(options);
        partOptimizationCheckVisitor.Start();

        for (ui32 i = 0; i < partsInfos.size(); ++i) {
            const TPartState& part = partsInfos[i];
            if (!part.Writable) {
                partOptimizationCheckVisitor.VisitPart(i, part);
            }
        }
        const TArchiveInfo::TOptimizationInfo optimizationInfo = partOptimizationCheckVisitor.GetOptimizationInfo();
        info.Optimizations.emplace(optimizationInfo.GetLabel(), optimizationInfo);
    }

    void TMultipartArchive::GetInfo(TArchiveInfo& info) {
        TVector<TPartState> partsInfo;
        {
            TReadGuard rg(Lock);
            partsInfo.reserve(Parts.size());
            for (auto& partPair: Parts) {
                const auto partPtr = partPair.second;
                if (Y_UNLIKELY(partPtr->GetDocsCount() == 0))
                    continue;

                const ui32 docsCount = partPtr->GetDocsCount();
                const ui32 headerSize = partPtr->GetDocsCount(true);
                const ui64 sizeInBytes = partPtr->GetSizeInBytes();

                partsInfo.emplace_back();
                partsInfo.back().DocsCount = docsCount;
                partsInfo.back().SizeInBytes = sizeInBytes;
                partsInfo.back().HeaderSize = headerSize;
                partsInfo.back().PartSizeLimit = partPtr->GetPartSizeLimit();
                partsInfo.back().Writable = partPtr->IsWritable();

                info.AliveDocsCount += docsCount;
                info.FullSizeInBytes += sizeInBytes;
                info.RemovedDocsCount += headerSize - docsCount;
            }
        }

        info.PartsCount = partsInfo.size();

        FillOptimization(0.1f, partsInfo, info);
        FillOptimization(0.2f, partsInfo, info);
        FillOptimization(0.3f, partsInfo, info);
        FillOptimization(0.4f, partsInfo, info);
        FillOptimization(0.5f, partsInfo, info);
        FillOptimization(0.6f, partsInfo, info);
        FillOptimization(0.7f, partsInfo, info);
    }


    TMultipartReadOnlyArchive* TMultipartReadOnlyArchive::Construct(const TFsPath& path, const TMultipartConfig& config, const NNamedLock::TNamedLockPtr& archiveLock) {
        TMemoryMapCommon::EOpenMode fatMode = TMemoryMapCommon::oRdOnly;
        if (config.PrechargeFAT) {
            fatMode |= TMemoryMapCommon::oPrecharge;
        }

        TAutoPtr<TFATMultipartReadOnly> fat = new TFATMultipartReadOnly(GetFatPath(path), fatMode);
        return new TMultipartReadOnlyArchive(path, fat.Release(), new TArchiveManager(config.CreateReadContext(), true, config.WritableThreadsCount), archiveLock);
    }

    TMultipartReadOnlyArchive::TMultipartReadOnlyArchive(
          const TFsPath& path
        , IFat* fat
        , IArchiveManager* manager
        , const NNamedLock::TNamedLockPtr& archiveLock
    )
        : TMultipartReadOnlyImpl<ui32>(fat, path, manager, archiveLock)
    {
        InitParts();
    }

    TBlob TMultipartReadOnlyArchive::GetDocument(ui32 id) const {
        return DoReadDocument(id);
    }
}
