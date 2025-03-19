#pragma once

#include "cell.h"
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/logger/global/global.h>

#include <kernel/multipart_archive/protos/archive.pb.h>

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/hash_set.h>
#include <util/folder/path.h>



struct THashHeader {
    ui32 Version = 0;
    ui32 BucketsCount = 0;
    ui64 FreeCellIndex = 0;

    THashHeader(ui32 bucketsC = 100000)
        : BucketsCount(bucketsC)
    {}

    void FromProto(const NRTYArchive::THashMetaInfo& info) {
        Version = info.GetVersion();
        BucketsCount = info.GetBucketsCount();
        FreeCellIndex = info.GetFreeCellIndex();
    }

    void ToProto(NRTYArchive::THashMetaInfo& info) {
        info.SetVersion(Version);
        info.SetBucketsCount(BucketsCount);
        info.SetFreeCellIndex(FreeCellIndex);
    }
};

template<class T>
class IHashDataStorage {
public:
    using TPtr = TAtomicSharedPtr<IHashDataStorage>;

    virtual ui64 Size() const = 0;
    virtual void Resize(ui64 size) = 0;
    virtual T& GetData(ui64 index) = 0;
    virtual const T& GetData(ui64 index) const = 0;
    virtual void Init(ui64 size) = 0;
    virtual ~IHashDataStorage() {}
};

template <class THash, class TValue>
class THashLogic {
public:
    using TCell = ::TCell<TValue>;
private:
    static const TCell BrokenCell;

    struct TCellInfo {
        ui64 Index;
        TCell& Cell;

        TCellInfo(ui64 index, const TCell& cell)
            : Index(index)
            , Cell(const_cast<TCell&>(cell)) {}

        TCellInfo(ui64 index, TCell& cell)
            : Index(index)
            , Cell(cell) {}

        inline explicit operator bool() const {
            return Index != UNKNOWN_POSITION;
        }

        static TCellInfo Empty() {
            return TCellInfo(UNKNOWN_POSITION, BrokenCell);
        }
    };

    typename IHashDataStorage<TCell>::TPtr Storage;
    THashHeader Header;
    ui64 FreeCellIndex;
    ui64 EmptyBucketsCount = 0;
    THashSet<ui64> DataCache;

public:
    THashLogic(typename IHashDataStorage<TCell>::TPtr storage, const THashHeader& header)
        : Header(header)
        , FreeCellIndex(header.FreeCellIndex)
    {
        Storage.Reset(storage);
        CHECK_WITH_LOG(Storage);

        if (FreeCellIndex == 0) {
            Storage->Init(header.BucketsCount + 1);
            FreeCellIndex = header.BucketsCount;
            Storage->GetData(FreeCellIndex) = TCell();
        } else {
            CHECK_WITH_LOG(!Storage->GetData(FreeCellIndex).IsInHash());
        }

        for (ui32 i = 0; i < Header.BucketsCount; ++i) {
            if (!Storage->GetData(i).IsInHash())
                EmptyBucketsCount++;
        }

        for (auto it = CreateIterator(); it.IsValid(); it.Next()) {
            DataCache.insert(it.Data().GetHash());
        }
    }

    ~THashLogic() {
        CHECK_WITH_LOG(!Storage->GetData(FreeCellIndex).IsInHash());
    }

    class THashIterator {
    public:
        THashIterator(typename IHashDataStorage<TCell>::TPtr storage, ui64 freeCellIndex)
            : Storage(storage)
            , FreeCellIndex(freeCellIndex) {
            Next();
        }

        bool IsValid() const {
            return Current < FreeCellIndex;
        }

        void Next() {
            Current++;
            while (Current < FreeCellIndex && !Storage->GetData(Current).IsInHash()) {
                Current++;
            }
        }

        const TCell& Data() const {
            return Storage->GetData(Current);
        }

    private:
        typename IHashDataStorage<TCell>::TPtr Storage;
        ui64 FreeCellIndex;
        ui64 Current = Max<ui64>();
    };

    class TListIterator {
    public:
        TListIterator(typename IHashDataStorage<TCell>::TPtr storage, ui64 start)
            : Storage(storage)
            , Start(start)
            , Position(Start) {
            if (!Storage->GetData(Position).IsInHash())
                Position = UNKNOWN_POSITION;
        }

        bool IsValid() const {
            return Position != UNKNOWN_POSITION;
        }

        void Next() {
            CHECK_WITH_LOG(IsValid());
            Position = Storage->GetData(Position).GetNext();
            if (Position == Start) {
                Position = UNKNOWN_POSITION;
            }
        }

        TCell& Data() {
            return Storage->GetData(Position);
        }

        ui64 Index() const {
            return Position;
        }

    private:
        typename IHashDataStorage<TCell>::TPtr Storage;
        ui64 Start;
        ui64 Position;
    };

private:
    TCellInfo FindCell(const THash& hash) const {
        TListIterator bucketIt (Storage, GetBucket(hash));

        if (!bucketIt.IsValid()) {
            return TCellInfo::Empty();
        }

        for (; bucketIt.IsValid(); bucketIt.Next()) {
            if (bucketIt.Data().GetHash() == hash) {
                return TCellInfo(bucketIt.Index(), bucketIt.Data());
            }
        }

        return TCellInfo::Empty();
    }

    TCellInfo GetLastCell() const {
        if (FreeCellIndex > Header.BucketsCount) {
            return TCellInfo(FreeCellIndex - 1, Storage->GetData(FreeCellIndex - 1));
        }

        return TCellInfo::Empty();
    }

    void Move(const TCellInfo& info) {
        CHECK_WITH_LOG(info.Index >= Header.BucketsCount);
        CHECK_WITH_LOG(!info.Cell.IsInHash());

        TCellInfo lastCell = GetLastCell();
        CHECK_WITH_LOG(lastCell);

        //Copy source
        Storage->GetData(info.Index) = lastCell.Cell;

        TListIterator bucketIt(Storage, GetBucket(lastCell.Cell.GetHash()));
        CHECK_WITH_LOG(bucketIt.IsValid());

        for (; bucketIt.IsValid(); bucketIt.Next()) {
            if (bucketIt.Data().GetNext() == FreeCellIndex - 1) {
                bucketIt.Data().SetNext(info.Index);
            }
        }

        MoveFreeCell(false);
    }

    void SwapData(ui64 idx1, ui64 idx2) {
        CHECK_WITH_LOG(idx1 >= Header.BucketsCount || idx2 >= Header.BucketsCount);
        CHECK_WITH_LOG(GetBucket(Storage->GetData(idx1).GetHash()) == GetBucket(Storage->GetData(idx2).GetHash()));

       Storage->GetData(idx1).Swap(Storage->GetData(idx2));
    }

    TCellInfo RemoveCellFromBucketList(const TCellInfo& info) {
        ui64 bDest = GetBucket(info.Cell.GetHash());

        if (info.Cell.GetNext() == info.Index) {
            DEBUG_LOG << "Clear bucket " << info.Index << Endl;
            CHECK_WITH_LOG(info.Index < Header.BucketsCount);
            Storage->GetData(info.Index).RemoveFromHash();
            EmptyBucketsCount++;
            return TCellInfo::Empty();
        }

        TListIterator bucketIt(Storage, bDest);
        CHECK_WITH_LOG(bucketIt.IsValid());

        if (info.Index == bDest) {
            DEBUG_LOG << "Remove bucket head " << info.Index << Endl;
            CHECK_WITH_LOG(info.Index == bucketIt.Index());
            SwapData(info.Index, info.Cell.GetNext());

            TCellInfo newInfo(info.Cell.GetNext(), Storage->GetData(info.Cell.GetNext()));
            bucketIt.Data().SetNext(newInfo.Cell.RemoveFromHash());
            return newInfo;
        }

        for (; bucketIt.IsValid(); bucketIt.Next()) {
            DEBUG_LOG << "Remove from bucket  " << bDest << Endl;
            if (bucketIt.Data().GetNext() == info.Index) {
                bucketIt.Data().SetNext(Storage->GetData(info.Index).RemoveFromHash());
                break;
            }
        }

        CHECK_WITH_LOG(!info.Cell.IsInHash());
        return info;
    }

    ui64 GetBucket(THash hash) const {
        return hash % Header.BucketsCount;
    }

    void MoveFreeCell(bool inc) {
        if (inc) {
            FreeCellIndex++;
        } else {
            FreeCellIndex--;
        }

        if (Storage->Size() <= FreeCellIndex) {
            Storage->Resize(2 * FreeCellIndex);
        }

        Storage->GetData(FreeCellIndex) = TCell();
    }

public:
    THashIterator CreateIterator() const {
        return THashIterator(Storage, FreeCellIndex);
    }

    bool Insert(const THash& hash, const TValue& val) {
        if (DataCache.contains(hash)) {
            TCellInfo old = FindCell(hash);
            CHECK_WITH_LOG(old);
            old.Cell.UpdateData(val);
            return false;
        }

        TCell cell(hash, val);

        ui64 bucketIdx = GetBucket(hash);

        TListIterator bucket(Storage, bucketIdx);
        ui64 cellIndex = bucketIdx;

        if (bucket.IsValid()) {
            cell.SetNext(bucket.Data().GetNext());
            cellIndex = FreeCellIndex;
            MoveFreeCell(true);
        } else {
            cell.SetNext(bucketIdx);
            cellIndex = bucketIdx;
            EmptyBucketsCount--;
        }

        Storage->GetData(cellIndex) = cell;
        Storage->GetData(bucketIdx).SetNext(cellIndex);
        DataCache.insert(hash);
        return true;
    }

    bool Remove(const THash& hash) {
        if (!DataCache.contains(hash)) {
            return false;
        }
        DataCache.erase(hash);

        TCellInfo info = FindCell(hash);
        CHECK_WITH_LOG(info);

        TCellInfo newInfo = RemoveCellFromBucketList(info);
        if (!!newInfo) {
            Move(newInfo);
        }

        return true;
    }

    bool Find(const THash& hash, TValue& val) const {
        if (!DataCache.contains(hash)) {
            return false;
        }

        TCellInfo info = FindCell(hash);
        CHECK_WITH_LOG(info);
        val = info.Cell.Data();
        return true;
    }

    ui64 Size() const {
        return FreeCellIndex - EmptyBucketsCount;
    }

    void SerializeStateToProto(NRTYArchive::THashMetaInfo& info) {
        Header.ToProto(info);
        info.SetFreeCellIndex(FreeCellIndex);
    }

    struct THashInfo {
        ui32 BucketsCount = 0;
        ui64 HashSize = 0;
        ui32 EmptyBucketsCount = 0;
        ui32 MinDocsPerBucket = 0;
        ui32 MaxDocsPerBucket = 0;

        void ToJson(NJson::TJsonValue& json) {
            CHECK_WITH_LOG(json.IsMap());

            json["buckets_count"] = BucketsCount;
            json["empty_buckets_count"] = EmptyBucketsCount;
            json["hash_size"] = HashSize;
            json["min_doc_per_bucket"] = MinDocsPerBucket;
            json["max_doc_per_bucket"] = MaxDocsPerBucket;
        }
    };

    ui32 GetBucketsCount() const {
        return Header.BucketsCount;
    }

    THolder<TListIterator> CreateListIterator(ui32 bucket) const {
        CHECK_WITH_LOG(bucket < GetBucketsCount());
        return MakeHolder<TListIterator>(Storage, bucket);
    }

    THashInfo GetHashInfo() const {
        THashInfo info;
        info.MaxDocsPerBucket = 0;
        info.MinDocsPerBucket = Max<ui32>();

        info.BucketsCount = Header.BucketsCount;
        info.HashSize = Size();
        info.EmptyBucketsCount = EmptyBucketsCount;

        TVector<ui32> stat(Header.BucketsCount, 0);
        for (ui32 i = 0; i < Header.BucketsCount; ++i) {
            if (Storage->GetData(i).IsInHash()) {
                TListIterator bucketIt(Storage, i);
                for (; bucketIt.IsValid(); bucketIt.Next()) {
                    stat[i]++;
                }

                info.MinDocsPerBucket = Min<ui32>(info.MinDocsPerBucket, stat[i]);
                info.MaxDocsPerBucket = Max<ui32>(info.MaxDocsPerBucket, stat[i]);
            }
        }

        return info;

    }
};

template<class THash, class TValue>
const typename THashLogic<THash, TValue>::TCell THashLogic<THash, TValue>::BrokenCell = THashLogic<THash, TValue>::TCell();
