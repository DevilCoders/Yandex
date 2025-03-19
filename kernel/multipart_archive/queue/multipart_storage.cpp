#include "multipart_storage.h"

#include <kernel/multipart_archive/abstract/globals.h>
#include <kernel/multipart_archive/iterators/factory.h>

#include <util/system/filemap.h>

namespace NRTYArchive {
    const ui32 TMultipartStorage::DOC_REMOVED = -1;
    const ui32 TMultipartStorage::DOC_NEW = 1;

    namespace {
        class TDummyFAT final : public IFat {
        public:
            explicit TDummyFAT(const TFsPath& path) {
                if (!path.Exists()) {
                    path.Touch();
                }
            }

            TPosition Get(size_t) const override {
                return TPosition::Removed();
            }

            ui64 Size() const override {
                return 0;
            }

            void Clear(ui64) override {
            }

            TPosition Set(size_t, TPosition) override {
                return TPosition::Removed();
            }

            IIterator::TPtr GetIterator() const override {
                return nullptr;
            }

            TVector<TPosition> GetSnapshot() const override {
                return {};
            }
        };

        class TRepairer : public TArchivePartThreadSafe::IRepairer {
        public:
            TRepairer(const TFsPath& brokenHeaderPath)
                : Position(0)
                , RepairedCount(0)
            {
                PartHeader.Init(brokenHeaderPath.GetPath().data());
            }

            void OnIterNext(ui32, ui64) override {
                if (Position == PartHeader.Size()) {
                    Lost++;
                    return;
                }

                if (PartHeader[Position] != TMultipartStorage::DOC_REMOVED) {
                    RepairedCount++;
                }

                Result.push_back(PartHeader[Position]);
                Position++;
            }

            const TVector<ui32>& GetHeader() const {
                PartHeader.Term();
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
                IPartHeader::SaveRestoredHeader(path, GetHeader());
            }
        private:
            TVector<ui32> Result;
            mutable TFileMappedArray<ui32> PartHeader;
            ui32 Position;
            ui32 RepairedCount;
            ui32 Lost = 0;
        };

        void CheckAndFix(const TFsPath& path, const IArchivePart::TConstructContext& ctx) {
            TVector<ui32> partIndexes;
            TMultipartStorage::FillPartsIndexes(path, partIndexes);
            for (ui32 i = 0; i < partIndexes.size(); ++i) {
                TFsPath headerPath = GetPartHeaderPath(path, partIndexes[i]);
                if (!headerPath.Exists()) {
                    WARNING_LOG << "Can't restore part " << headerPath << " cause its header absents" << Endl;
                    TArchivePartThreadSafe::Remove(path, partIndexes[i]);
                    continue;
                }

                if (TArchivePartThreadSafe::AllRight(path, partIndexes[i])) {
                    continue;
                }

                if (TPartMetaSaver(GetPartMetaPath(path, partIndexes[i]), /*forceOpen*/ false, /*allowUnknownFields*/ true)->GetStage() == TPartMetaInfo::OPENED) {
                    INFO_LOG << "Repair meta for " << GetPartPath(path, partIndexes[i]) << Endl;
                    TFileMappedArray<ui32> header;
                    header.Init(headerPath.c_str());
                    ui64 removedCount = 0;
                    for (ui32 doc = 0; doc < header.size(); ++doc) {
                        if (header[doc] == TMultipartStorage::DOC_REMOVED)
                            ++removedCount;
                    }
                    TPartMetaSaver meta(GetPartMetaPath(path, partIndexes[i]), /*forceOpen*/ false, /*allowUnknownFields*/ true);
                    meta->SetDocsCount(header.size());
                    meta->SetRemovedDocsCount(removedCount);
                    meta->SetStage(TPartMetaInfo::CLOSED);
                    continue;
                }

                TRepairer repairer(headerPath);
                if (!TArchivePartThreadSafe::Repair(path, partIndexes[i], ctx, &repairer)) {
                    WARNING_LOG << "Can't restore part " << headerPath << Endl;
                    TArchivePartThreadSafe::Remove(path, partIndexes[i]);
                }
            }
        }
    }

    TMultipartStorage::TMultipartStorage(const TFsPath& path, const IArchivePart::TConstructContext& partsCtx)
        : TBase(path, new TDummyFAT(GetFatPath(path)), new TArchiveManager(partsCtx), nullptr)
    {
        CheckAndFix(path, partsCtx);
        InitParts();

        TVector<TArchivePartThreadSafe::TPtr> partsToRemove;
        for (auto& partIt : Parts) {
            TArchivePartThreadSafe::TPtr part = partIt.second;
            if (part->IsWritable())
                continue;

            if (part->GetDocsCount() == 0) {
                WARNING_LOG << "Part is empty " << part->GetPath() << Endl;
                partsToRemove.push_back(part);
                continue;
            }

            if (!TRawIterator(part).IsValid()) {
                INFO_LOG << "Dropping part with broken data" << Endl;
                partsToRemove.push_back(part);
                continue;
            }
        }

        for (auto& part : partsToRemove) {
            Parts.erase(part->GetPartNum());
        }
    }

    TPosition TMultipartStorage::AppendDocument(const TBlob& document) {
        return PutDocument(document, DOC_NEW);
    }

    TDocInfo TMultipartStorage::AppendDocumentWithAddress(const TBlob& document) {
        ui32 idx = 0;
        TPosition result = PutDocument(document, DOC_NEW, 0, &idx);
        return TDocInfo(result.GetPart(), idx);
    }

    bool TMultipartStorage::RemoveDocument(const TDocInfo& address) {
        return RemoveDocument(address.GetPartIdx(), address.GetDocIdx());
    }

    bool TMultipartStorage::RemoveDocument(ui32 partNum, ui64 index) {
        const auto onDocRemove = [index](TArchivePartThreadSafe::TPtr part) {
            part->UpdateHeader(index, DOC_REMOVED);
        };

        RemoveDocumentFromPart(partNum, onDocRemove);
        return true;
    }
}
