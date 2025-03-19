#pragma once

#include <kernel/multipart_archive/common/hash.h>
#include <kernel/multipart_archive/archive_impl/archive_manager.h>
#include <kernel/multipart_archive/archive_impl/multipart_base.h>
#include <kernel/multipart_archive/common/hash_storage.h>

#include <util/generic/ptr.h>
#include <util/digest/fnv.h>
#include <util/generic/cast.h>
#include <util/generic/maybe.h>

namespace NRTYArchive {

    using TPositionsStorage = TMappedHash<TCell<NRTYArchive::TPosition>>;

    class THashManager: public IArchiveManager {
    public:
        THashManager(const IArchivePart::TConstructContext& constructCtx, bool readOnly)
            : IArchiveManager(constructCtx)
        {
            SetReadOnly(readOnly);
        }

        virtual IPartHeader* CreateHeader(const TFsPath& path, EOpenMode mode) const override {
            return new TFakePartHeader(path, mode);
        }

        virtual TAtomicSharedPtr<TPartMetaSaver> CreateMetaSaver(const TFsPath& path) const override {
            return new TPartMetaSaver(path, /*forceOpen*/ false, /*allowUnknownFields*/ true);
        }
    };

    class THashFAT final : public IFat {
        using TKey = ui64;
    public:
        using THashMetaSaver = TProtoFileGuard<NRTYArchive::THashMetaInfo>;
        using TMultipartHashLogic = THashLogic<TKey, TPosition>;
        using TListIterator = TMultipartHashLogic::TListIterator;
    private:

        class THashIterator final : public IFat::IIterator {
        public:
            THashIterator(typename TMultipartHashLogic::THashIterator slave)
                : Slave(slave) {}

            bool IsValid() const override {
                return Slave.IsValid();
            }

            size_t GetId() const override {
                return Slave.Data().GetHash();
            }

            void Next() override {
                Slave.Next();
            }

            TPosition GetPosition() const override {
                return Slave.Data().Data();
            }

        private:
            typename TMultipartHashLogic::THashIterator Slave;
        };

    public:
        THashFAT(const TFsPath& path, TMaybe<ui32> buckets, bool readOnly = false)
            : Path(path)
            , ReadOnly(readOnly)
        {
            TFsPath headerPath = path.GetPath() + ".meta";
            THashHeader header;
            if (!headerPath.Exists()) {
                CHECK_WITH_LOG(!ReadOnly);
                if (buckets.Defined()) {
                    if (header.FreeCellIndex == 0) {
                        header.BucketsCount = buckets.GetRef();
                    } else {
                        CHECK_WITH_LOG(header.BucketsCount == buckets.GetRef());
                    }
                }
                THashMetaSaver meta(headerPath);
                header.ToProto(*meta);
            } else {
                const THashMetaSaver meta(headerPath);
                header.FromProto(*meta);
            }

            IndexHash.Reset(new TMultipartHashLogic(new TPositionsStorage(GetFatPath(path).GetPath(), readOnly), header));
        }

        TPosition Get(size_t key) const override {
            TPosition pos = TPosition::Removed();
            TReadGuard g(Lock);
            IndexHash->Find(key, pos);
            return pos;
        }

        TPosition Set(size_t key, TPosition position) override {
            TPosition oldPos = TPosition::Removed();
            TWriteGuard g(Lock);
            IndexHash->Find(key, oldPos);
            if (position.IsRemoved()) {
                IndexHash->Remove(key);
            } else {
                IndexHash->Insert(key, position);
            }
            return oldPos;
        }

        ui64 Size() const override {
            TReadGuard g(Lock);
            return IndexHash->Size();
        }

        void Clear(ui64) override {
            FAIL_LOG("Not implemented");
        }

        IIterator::TPtr GetIterator() const override {
            return MakeHolder<THashIterator>(IndexHash->CreateIterator());
        }

        TVector<TPosition> GetSnapshot() const override {
            return {};
        }

        const TMultipartHashLogic& GetHashLogic() const {
            return *IndexHash;
        }

        ~THashFAT() {
            if (!ReadOnly) {
                THashMetaSaver meta(Path.GetPath() + ".meta");
                IndexHash->SerializeStateToProto(*meta);
            }
        }

    private:
        void Resize(ui32) {
            FAIL_LOG("Not valid");
        }

    private:
        THolder<TMultipartHashLogic> IndexHash;
        TFsPath Path;
        TRWMutex Lock;
        bool ReadOnly;
    };

    class TMultipartHashImpl: public TMultipartImpl<ui64> {
        using TBase = TMultipartImpl<ui64>;
        using TKey = ui64;

    public:
        using TPartMetaSaver = TProtoFileGuard<NRTYArchive::TPartMetaInfo>;

        static void Remove(const TFsPath& path) {
            TVector<ui32> partIndexes;
            FillPartsIndexes(path, partIndexes);
            for (ui64 index : partIndexes) {
                TArchivePartThreadSafe::Remove(path, index);
            }
            if (GetFatPath(path).Exists()) {
                CHECK_WITH_LOG(NFs::Remove(GetFatPath(path)));
            }
            NFs::Remove(path.GetPath() + ".meta");
        }

        TMultipartHashImpl(const TFsPath& path, const IArchivePart::TConstructContext& constructCtx, TMaybe<ui32> buckets, bool readOnly = false)
            : TBase(path, new THashFAT(path, buckets, readOnly), new THashManager(constructCtx, readOnly), nullptr)
        {
            InitCommon();
        }

        static bool AllRight(const TFsPath& path) {
            TVector<ui32> parts;
            FillPartsIndexes(path, parts);
            for (const auto& index : parts) {
                if (!TArchivePartThreadSafe::AllRight(path, index)) {
                    WARNING_LOG << "Broken part " << GetPartHeaderPath(path, index) << Endl;
                    return false;
                }
            }
            return true;
        }

        class TRepairer: public TArchivePartThreadSafe::IRepairer {
        public:
            using TCellsByOffset = TMap<ui64, THashFAT::TMultipartHashLogic::TCell>;

            TRepairer(const TCellsByOffset& cellByOffset, THashFAT::TMultipartHashLogic& hash)
                : CellByOffset(cellByOffset)
                , HashLogic(hash)
            {}

            void OnIterNext(ui32, ui64 offset) override {
                AllCount++;
                auto it = CellByOffset.find(offset);
                if (it != CellByOffset.end()) {
                    RepairedCount++;
                    const THashFAT::TMultipartHashLogic::TCell& cell = it->second;
                    HashLogic.Insert(cell.GetHash(), cell.Data());
                }
            }

            ui32 GetFullDocsCount() const override {
                return AllCount;
            }

            ui32 GetRepairedDocsCount() const override {
                return RepairedCount;
            }

            virtual ui32 GetLost() const override {
                return AllCount - RepairedCount;
            }

        private:
            ui32 RepairedCount = 0;
            ui32 AllCount = 0;
            const TCellsByOffset& CellByOffset;
            THashFAT::TMultipartHashLogic& HashLogic;
        };

        static void Repair(const TFsPath& path, const IArchivePart::TConstructContext& ctx) {
            TVector<ui32> parts;
            FillPartsIndexes(path, parts);
            TMap<ui64, TRepairer::TCellsByOffset> offsets;

            for (const auto& index : parts) {
                offsets[index] = TRepairer::TCellsByOffset();
            }

            THashHeader newHeader;
            {
                THashHeader header;
                const THashFAT::THashMetaSaver meta(path.GetPath() + ".meta");
                header.FromProto(*meta);
                newHeader.BucketsCount = header.BucketsCount;
            }

            TFsPath tmpPath(GetFatPath(path).GetPath() + ".tmp");
            tmpPath.ForceDelete();

            TFileMappedArray<THashFAT::TMultipartHashLogic::TCell> hashData;
            hashData.Init(GetFatPath(path).c_str());

            for (ui32 i = 0; i < hashData.size(); ++i) {
                const THashFAT::TMultipartHashLogic::TCell& cell = hashData[i];
                if (cell && cell.GetNext() != UNKNOWN_POSITION) {
                    CHECK_WITH_LOG(!cell.Data().IsRemoved());
                    if (offsets.contains(cell.Data().GetPart())) {
                        offsets[cell.Data().GetPart()][cell.Data().GetOffset()] = cell;
                    } else {
                        ERROR_LOG << "No data for part " << cell.Data().GetPart() << "/" << cell.Data().IsRemoved() << Endl;
                    }
                }
            }

            THashFAT::TMultipartHashLogic hash(new TPositionsStorage(tmpPath), newHeader);

            for (auto& part : offsets) {
                if (part.second.size() == 0) {
                    ERROR_LOG << "Remove empty part " << GetPartPath(path, part.first) << Endl;
                    TArchivePartThreadSafe::Remove(path, part.first);
                } else {
                    const TPartMetaInfo::TStage stage = TPartMetaSaver(GetPartMetaPath(path, part.first), /*forceOpen*/ false, /*allowUnknownFields*/ true)->GetStage();
                    if (stage != TPartMetaInfo::CLOSED && stage != TPartMetaInfo::OPENED) {
                        TRepairer repairer(part.second, hash);
                        if (!TArchivePartThreadSafe::Repair(path, part.first, ctx, &repairer)) {
                            WARNING_LOG << "Can't restore part " << part.first << Endl;
                            TArchivePartThreadSafe::Remove(path, part.first);
                            continue;
                        }
                    } else {
                        for (auto&& cell : part.second) {
                            hash.Insert(cell.second.GetHash(), cell.second.Data());
                        }
                        TPartMetaSaver meta(GetPartMetaPath(path, part.first), /*forceOpen*/ false, /*allowUnknownFields*/ true);
                        CHECK_WITH_LOG(meta->GetDocsCount() >= part.second.size()) << "Inconsistent part " << GetPartPath(path, part.first) << meta->GetDocsCount() << "/" << part.second.size();
                        meta->SetRemovedDocsCount(meta->GetDocsCount() - part.second.size());
                        meta->SetStage(TPartMetaInfo::CLOSED);
                    }
                }
            }

            {
                THashFAT::THashMetaSaver meta(path.GetPath() + ".meta");
                hash.SerializeStateToProto(*meta);
            }
            tmpPath.ForceRenameTo(GetFatPath(path));

        }

        template <class TVisitor>
        void ScanHashLists(TVisitor& visitor) const {
            THashFAT* hashFat = VerifyDynamicCast<THashFAT*>(FAT.Get());
            const THashFAT::TMultipartHashLogic& innerHash = hashFat->GetHashLogic();

            ui32 bucketsCount = innerHash.GetBucketsCount();

            for (ui32 bucket = 0; bucket < bucketsCount; ++bucket) {
                THolder<THashFAT::TListIterator> listIterator = innerHash.CreateListIterator(bucket);

                if (!listIterator->IsValid()) {
                    continue;
                }

                if (!visitor.OnBucket(bucket)) {
                    continue;
                }

                while (listIterator->IsValid()) {
                    TPosition pos = listIterator->Data().Data();
                    ui64 hash = listIterator->Data().GetHash();
                    if (!visitor.OnItem(hash, GetDocumentByPosition(pos))) {
                        break;
                    }
                    listIterator->Next();
                }
            }
        }

        ui32 GetBucketsCount() {
            THashFAT* hashFat = VerifyDynamicCast<THashFAT*>(FAT.Get());
            const THashFAT::TMultipartHashLogic& innerHash = hashFat->GetHashLogic();
            return innerHash.GetBucketsCount();
        }
    };

    class TMultipartHash: public TMultipartHashImpl {
    public:
        using TPtr = TAtomicSharedPtr<TMultipartHash>;

        TMultipartHash(const TFsPath& path, const IArchivePart::TConstructContext& constructCtx, TMaybe<ui32> buckets, bool readOnly = false)
            : TMultipartHashImpl(path, constructCtx, buckets, readOnly) {}

        bool Insert(const TString& key, TBlob data) {
            PutDocument(data, GetHash(key));
            return true;
        }

        bool Remove(const TString& key) {
            return RemoveDocument(GetHash(key));
        }

        TBlob Find(const TString& key) const {
            return ReadDocument(GetHash(key));
        }

        bool Find(const TString& key, TBlob& data) const {
            data = ReadDocument(GetHash(key));
            return !data.Empty();
        }

        void GetHashInfo(NJson::TJsonValue& json) const {
            THashFAT* fat = dynamic_cast<THashFAT*>(FAT.Get());
            CHECK_WITH_LOG(fat);
            auto info = fat->GetHashLogic().GetHashInfo();
            info.ToJson(json);
        }

        ui64 Size() const {
            return GetDocsCount(true);
        }

        ui64 GetHash(const TString& key) const {
            return FnvHash<ui64>(key);
        }
    };

    using TMultipartHashLists = TMultipartHashImpl;
}
