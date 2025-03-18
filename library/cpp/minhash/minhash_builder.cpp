#include "minhash_builder.h"
#include "minhash_func.h"
#include "minhash_helpers.h"
#include "iterators.h"
#include "prime.h"

#include <library/cpp/succinct_arrays/intvector.h>

#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/hash_set.h>
#include <util/system/yassert.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitops.h>
#include <util/stream/buffer.h>
#include <util/folder/dirut.h>

namespace NMinHash {
    using namespace NSuccinctArrays;

    struct TMapItem {
        ui32 Bid;
        TItem Item;
    };

    struct TBucket {
        ui32 Id;
        ui32 Size;
        ui32 Offset;
    };

    bool CmpBucketBySize(const TBucket& x, const TBucket& y) {
        return x.Size > y.Size;
    }

    class TChdHashBuilder::TImpl {
    public:
        typedef TChdMinHashFunc THash;

    public:
        TImpl(ui32 numKeys, double loadFactor, ui32 keysPerBucket, ui32 seed, ui8 fprSize);
        template <typename C>
        void Build(const C& cont, IOutputStream* out);
        template <typename C>
        TAutoPtr<TChdMinHashFunc> Build(const C& cont);
        ui32 ResetSeed() {
            Seed_ = RandomNumber<ui32>();
            return Seed_;
        }

    private:
        static ui32 NumBins(ui32 numKeys, double loadFactor) {
            ui32 n = static_cast<ui32>(numKeys / loadFactor) + 1;
            n = IsPrime(n) ? n : NextPrime(n);
            return n;
        }
        template <typename C>
        bool Map(const C& cont);
        void Order();
        bool Search();
        void Minimize();
        template <typename C>
        ui32 HashItems(const C& cont, TVector<TMapItem>& mapItems, TVector<TBucket>& buckets);
        bool PlaceItems(const TVector<TMapItem>& mapItems, TVector<TItem>& items, TVector<TBucket>& buckets);
        bool PlaceBucket(const TBucket& bucket);
        bool ProbeBucket(const TBucket& bucket, ui32 probe0, ui32 probe1);
        void AddFpr();
        ui32 GetPos(ui32 displ, ui32 first, ui32 second) const;
        void Save(IOutputStream* out) const;

    private:
        static const ui32 MAX_ITER = 1000;
        static const ui32 MAX_MAP_ITER = 1000;

        ui32 NumKeys_;
        double LoadFactor_;
        ui32 Seed_;
        ui32 KeysPerBucket_;
        ui8 FprSize_;
        ui32 NumBuckets_;
        ui32 NumBins_;
        ui32 MaxProbeIter_;

        TBitVector<ui64> FreeBins_;
        TVector<ui32> Displ_;
        TVector<ui32> BinRank_;
        TVector<ui32> Fpr_;

        TVector<TBucket> Buckets_;
        TVector<TItem> Items_;

        TVector<ui32> Positions_;
        ui32 MaxBucketSize_;

        friend class TSerializer<NMinHash::TChdHashBuilder::TImpl>;
    };

    TChdHashBuilder::TImpl::TImpl(ui32 numKeys, double loadFactor, ui32 keysPerBucket, ui32 seed, ui8 fprSize)
        : NumKeys_(numKeys)
        , LoadFactor_(loadFactor < 0.5 ? 0.5 : loadFactor > 0.999999 ? 0.999999 : loadFactor)
        , Seed_(seed ? seed : RandomNumber<ui32>(NumKeys_))
        , KeysPerBucket_(keysPerBucket)
        , FprSize_(fprSize > 32 ? 32 : fprSize)
        , NumBuckets_(NumKeys_ / KeysPerBucket_ + 1)
        , NumBins_(NumBins(NumKeys_, LoadFactor_))
        , MaxProbeIter_(static_cast<ui32>((Log2(NumKeys_ + 2) / 20) * (1 << 20)))
        , MaxBucketSize_(0)
    {
    }

    template <typename C>
    void TChdHashBuilder::TImpl::Build(const C& cont, IOutputStream* out) {
        TAutoPtr<TChdMinHashFunc> hash = Build(cont);
        hash->SaveLoad(out);
    }

    template <typename C>
    TAutoPtr<TChdMinHashFunc> TChdHashBuilder::TImpl::Build(const C& cont) {
        ui32 numIter = MAX_ITER;
        for (; numIter > 0; --numIter) {
            if (!Map(cont)) {
                ythrow THashBuildException() << "failed to map items";
            }
            Order();
            if (Search()) {
                break;
            }
        }
        if (!numIter) {
            ythrow THashBuildException() << "failed to build hash";
        }
        Minimize();
        AddFpr();

        TChdMinHashFunc* hash = new TChdMinHashFunc;
        DoSwap(NumBins_, hash->NumBins_);
        DoSwap(Seed_, hash->Seed_);
        THash::TDispl(Displ_.begin(), Displ_.end()).Swap(hash->Displ_);
        THash::TRank(BinRank_.begin(), BinRank_.end()).Swap(hash->BinRank_);
        THash::TFpr(Fpr_.begin(), Fpr_.end(), FprSize_).Swap(hash->Fpr_);
        DoSwap(FprSize_, hash->FprSize_);
        return hash;
    }

    template <typename C>
    bool TChdHashBuilder::TImpl::Map(const C& cont) {
        TVector<TMapItem> mapItems(NumKeys_, TMapItem());
        for (ui32 numIter = MAX_MAP_ITER; numIter > 0; --numIter) {
            MaxBucketSize_ = HashItems(cont, mapItems, Buckets_);
            if (PlaceItems(mapItems, Items_, Buckets_)) {
                TVector<TMapItem>().swap(mapItems);
                return true;
            }
            ResetSeed();
        }
        return false;
    }

    void TChdHashBuilder::TImpl::Order() {
        StableSort(Buckets_.begin(), Buckets_.end(), CmpBucketBySize);
        for (size_t i = Buckets_.size(); i > 0; --i) {
            if (Buckets_[i - 1].Size)
                break;
            Buckets_.erase(Buckets_.begin() + i - 1);
        }
    }

    bool TChdHashBuilder::TImpl::Search() {
        TVector<ui32>(MaxBucketSize_).swap(Positions_);
        TVector<ui32>(NumBuckets_).swap(Displ_);
        TBitVector<ui64>(NumBins_).Swap(FreeBins_);
        for (size_t i = 0; i < Buckets_.size(); ++i) {
            if (!PlaceBucket(Buckets_[i])) {
                ResetSeed();
                return false;
            }
        }
        return true;
    }

    void TChdHashBuilder::TImpl::Minimize() {
        BinRank_.clear();
        for (size_t i = 0; i < FreeBins_.Size(); ++i) {
            if (!FreeBins_.Test(i))
                BinRank_.push_back(i);
        }
    }

    template <typename C>
    ui32 TChdHashBuilder::TImpl::HashItems(const C& cont, TVector<TMapItem>& mapItems, TVector<TBucket>& buckets) {
        mapItems.clear();
        mapItems.resize(NumKeys_);
        buckets.clear();
        buckets.resize(NumBuckets_);
        size_t i = 0;
        for (typename C::const_iterator it = cont.begin(); it != cont.end() && i < mapItems.size(); ++it, ++i) {
            TMapItem& mi = mapItems[i];
            mi.Bid = TChdMinHashFunc::BaseHash(it->data(), it->size(), Seed_, NumBins_, NumBuckets_, &mi.Item);
            buckets[mi.Bid].Size++;
        }
        if (i != mapItems.size()) {
            ythrow yexception() << "invalid number of keys; expected " << NumKeys_ << " got " << i;
        }
        buckets[0].Offset = 0;
        buckets[0].Id = 0;
        ui32 maxBucketSize = buckets[0].Size;
        for (i = 1; i < buckets.size(); ++i) {
            buckets[i].Id = i;
            buckets[i].Offset = buckets[i - 1].Offset + buckets[i - 1].Size;
            buckets[i - 1].Size = 0;
            maxBucketSize = Max<ui32>(maxBucketSize, buckets[i].Size);
        }
        buckets[i - 1].Size = 0;
        return maxBucketSize;
    }

    bool TChdHashBuilder::TImpl::PlaceItems(const TVector<TMapItem>& mapItems, TVector<TItem>& items, TVector<TBucket>& buckets) {
        items.clear();
        items.resize(mapItems.size());
        for (size_t i = 0; i < mapItems.size(); ++i) {
            const TMapItem& mi = mapItems[i];
            TBucket& bucket = buckets[mi.Bid];
            TItem* item = &items[bucket.Offset];
            for (ui32 j = 0; j < bucket.Size; ++j, ++item) {
                if (item->First == mi.Item.First && item->Second == mi.Item.Second) {
                    return false;
                }
            }
            item->First = mi.Item.First;
            item->Second = mi.Item.Second;
            item->Fpr = mi.Item.Fpr;
            bucket.Size++;
        }
        return true;
    }

    bool TChdHashBuilder::TImpl::PlaceBucket(const TBucket& bucket) {
        if (bucket.Size == 0)
            return true;
        ui32 probe1 = 0;
        ui32 probe0 = 0;
        ui32 probe = 0;
        do {
            if (ProbeBucket(bucket, probe0, probe1)) {
                Displ_[bucket.Id] = probe0 + probe1 * NumBins_;
                return true;
            }
            probe0++;
            if (probe0 >= NumBins_) {
                probe0 -= NumBins_;
                probe1++;
            }
            probe++;
        } while (!(probe >= MaxProbeIter_ || probe1 >= NumBins_));
        return false;
    }

    bool TChdHashBuilder::TImpl::ProbeBucket(const TBucket& bucket, ui32 probe0, ui32 probe1) {
        for (size_t i = 0; i < bucket.Size; ++i) {
            const TItem& item = Items_[bucket.Offset + i];
            ui32 pos = TChdMinHashFunc::GetBinPos(NumBins_, item.First, item.Second, probe0, probe1);
            if (!FreeBins_.Set(pos)) {
                for (size_t j = 0; j < i; ++j)
                    FreeBins_.Reset(Positions_[j]);
                return false;
            }
            Positions_[i] = pos;
        }
        return true;
    }

    void TChdHashBuilder::TImpl::AddFpr() {
        if (FprSize_) {
            Fpr_.assign(BinRank_.size() ? NumBins_ - BinRank_.size() : NumBins_, 0);
            for (size_t i = 0; i < Buckets_.size(); ++i) {
                const TBucket& bucket = Buckets_[i];
                for (size_t j = 0; j < bucket.Size; ++j) {
                    const TItem& item = Items_[bucket.Offset + j];
                    ui32 pos = GetPos(Displ_[bucket.Id], item.First, item.Second);
                    Fpr_[pos] = item.Fpr & MaskLowerBits(FprSize_);
                }
            }
        }
    }

    ui32 TChdHashBuilder::TImpl::GetPos(ui32 displ, ui32 first, ui32 second) const {
        ui32 pos = TChdMinHashFunc::GetBinPos(NumBins_, first, second, displ % NumBins_, displ / NumBins_);
        if (BinRank_.size()) {
            pos -= (LowerBound(BinRank_.begin(), BinRank_.end(), pos) - BinRank_.begin());
        }
        return pos;
    }

    void TChdHashBuilder::TImpl::Save(IOutputStream* out) const {
        TChdMinHashFuncHdr hdr(THash::Type);
        ::Save(out, hdr);
        ::Save(out, NumBins_);
        ::Save(out, Seed_);
        ::Save(out, THash::TDispl(Displ_.begin(), Displ_.end()));
        ::Save(out, THash::TRank(BinRank_.begin(), BinRank_.end()));
        ::Save(out, FprSize_);
        ::Save(out, THash::TFpr(Fpr_.begin(), Fpr_.end(), FprSize_));
    }

    TChdHashBuilder::TChdHashBuilder(ui32 numKeys, double loadFactor, ui32 keysPerBucket, ui32 seed, ui8 fprSize)
        : Impl_(new TImpl(numKeys, loadFactor, keysPerBucket, seed, fprSize))
    {
    }

    TChdHashBuilder::~TChdHashBuilder() {
        delete Impl_;
    }

    template <typename C>
    void TChdHashBuilder::Build(const C& cont, IOutputStream* out) {
        Impl_->Build(cont, out);
    }
    template <typename C>
    TAutoPtr<TChdMinHashFunc> TChdHashBuilder::Build(const C& cont) {
        return Impl_->Build(cont);
    }

    template void TChdHashBuilder::Build(const TVector<TString>& cont, IOutputStream* out);

    template void TChdHashBuilder::Build(const TVector<TStringBuf>& cont, IOutputStream* out);

    template void TChdHashBuilder::Build(const TSet<TString>& cont, IOutputStream* out);

    template void TChdHashBuilder::Build(const THashSet<TString>& cont, IOutputStream* out);

    template void TChdHashBuilder::Build(const TKeysContainer& cont, IOutputStream* out);

    template TAutoPtr<TChdMinHashFunc> TChdHashBuilder::Build(const TVector<TString>& cont);

    template TAutoPtr<TChdMinHashFunc> TChdHashBuilder::Build(const TVector<TStringBuf>& cont);

    template TAutoPtr<TChdMinHashFunc> TChdHashBuilder::Build(const TKeysContainer& cont);

    template TAutoPtr<TChdMinHashFunc> TChdHashBuilder::Build(const TSet<TString>& cont);

    template TAutoPtr<TChdMinHashFunc> TChdHashBuilder::Build(const THashSet<TString>& cont);

    TDistChdHashBuilder::TDistChdHashBuilder(ui32 numPortions, double loadFactor, ui32 keysPerBucket, ui8 fprSize, IOutputStream* out)
        : NumPortions_(numPortions)
        , LoadFactor_(loadFactor < 0.5 ? 0.5 : loadFactor > 0.99 ? 0.99 : loadFactor)
        , KeysPerBucket_(keysPerBucket)
        , FprSize_(fprSize > 32 ? 32 : fprSize)
        , Out_(out)
    {
        TChdMinHashFuncHdr hdr(TDistChdMinHashFunc::Type);
        ::Save(Out_, hdr);
        ::SaveSize(Out_, NumPortions_);
    }

    template <typename C>
    void TDistChdHashBuilder::Add(const C& cont) {
        TChdHashBuilder builder(cont.size(), LoadFactor_, KeysPerBucket_, 0, FprSize_);
        builder.Build(cont, Out_);
        Off_.push_back(cont.size());
    }

    void TDistChdHashBuilder::Add(IZeroCopyInput* inp, ui32 size) {
        TransferData(inp, Out_);
        Off_.push_back(size);
    }

    void TDistChdHashBuilder::Add(const TChdMinHashFunc& hash) {
        ::Save(Out_, hash);
        Off_.push_back(hash.Size());
    }

    void TDistChdHashBuilder::Finish() {
        Y_VERIFY(Off_.size() == NumPortions_, " ");
        ui64 prevSize = Off_[0];
        Off_[0] = 0;
        for (size_t i = 1; i < Off_.size(); ++i) {
            ui64 tmp = Off_[i];
            Off_[i] = Off_[i - 1] + prevSize;
            prevSize = tmp;
        }
        ::Save(Out_, TDistChdMinHashFunc::TOff(Off_.begin(), Off_.end()));
    }

    template void TDistChdHashBuilder::Add(const TVector<TString>& cont);

    template void TDistChdHashBuilder::Add(const TVector<TStringBuf>& cont);

    template void TDistChdHashBuilder::Add(const TKeysContainer& cont);

    void CreateHash(const TString& fileName, double loadFactor, ui32 numKeys, ui32 keysPerBucket,
                    ui32 seed, ui8 errorBits, bool wide, IOutputStream* out) {
        TChdHashBuilder builder(numKeys, loadFactor, keysPerBucket, seed, errorBits);
        TKeysContainer keys(fileName, wide);
        builder.Build(keys, out);
    }

    void CreateDistHash(const TString& inpFileName, const TString& outFileName, ui32 numPortions,
                        double loadFactor, ui32 keysPerBucket, ui8 errorBits, bool wide) {
        char path[FILENAME_MAX];
        if (MakeTempDir(path, nullptr))
            ythrow yexception() << "Failed to create a tmp dir";
        TString dir = path;

        TVector<TString> fileNames;
        for (size_t i = 0; i < numPortions; ++i) {
            fileNames.push_back(TString::Join(dir, "minhash.part.", ToString<size_t>(i)));
        }

        {
            TVector<TAutoPtr<TFixedBufferFileOutput>> files;
            for (size_t i = 0; i < fileNames.size(); ++i) {
                files.push_back(new TFixedBufferFileOutput(fileNames[i]));
            }
            TKeysContainer keys(inpFileName, wide);
            for (TKeysContainer::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                ui32 i = TDistChdMinHashFunc::Bucket(it->data(), it->size(), numPortions);
                (*files[i]) << it.Line() << '\n';
            }
        }

        {
            TFixedBufferFileOutput fo(outFileName);
            TDistChdHashBuilder builder(numPortions, loadFactor, keysPerBucket, errorBits, &fo);
            for (size_t i = 0; i < fileNames.size(); ++i) {
                TKeysContainer keys(fileNames[i], wide);
                builder.Add(keys);
            }
            builder.Finish();
            RemoveDirWithContents(dir);
        }

        {
            TFileInput fi(outFileName);
            TDistChdMinHashFunc hash(&fi);

            TKeysContainer keys(inpFileName, wide);
            for (TKeysContainer::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                Cerr << *it << '\t' << hash.Get(it->data(), it->size()) << Endl;
            }
        }
    }

}
