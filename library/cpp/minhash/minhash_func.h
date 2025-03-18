#pragma once

#include "minhash_helpers.h"

#include <library/cpp/succinct_arrays/intvector.h>
#include <library/cpp/succinct_arrays/eliasfano.h>

#include <util/system/defaults.h>
#include <util/generic/bitops.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/ysaveload.h>
#include <util/stream/file.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/generic/noncopyable.h>

namespace NMinHash {
    using namespace NSuccinctArrays;

    class TChdMinHashFunc : TNonCopyable {
    public:
        typedef TStringBuf TKey;
        typedef ui32 TDomain;
        typedef TEliasFanoArray<ui32> TDispl;
        typedef TVectorRank TRank;
        typedef TIntVector TFpr;

        static int Type;
        static const TDomain npos = static_cast<ui32>(-1);

        static ui32 GetBinPos(ui32 numBins, ui32 first, ui32 second, ui32 probe0, ui32 probe1) {
            return static_cast<ui32>((first + static_cast<ui64>(second) * probe0 + probe1) % numBins);
        }

        static ui32 BaseHash(const void* buf, size_t len, ui32 seed, ui32 numBins, ui32 numBuckets, TItem* res) {
            uint128 h;
            h.first = seed;
            h = CityHash128WithSeed((const char*)buf, len, h);
            res->First = ((h.first >> 32) & 0xFFFFFFFF) % numBins;
            res->Second = (h.second & 0xFFFFFFFF) % (numBins - 1) + 1;
            res->Fpr = ((h.second >> 32) & 0xFFFFFFFF);
            return (h.first & 0xFFFFFFFF) % numBuckets;
        }

        TChdMinHashFunc()
            : NumBins_()
            , Seed_()
            , FprSize_()
        {
        }

        explicit TChdMinHashFunc(IInputStream* inp) {
            SaveLoad(inp);
        }

        explicit TChdMinHashFunc(const TString& fileName) {
            TAutoPtr<IInputStream> inp = OpenInput(fileName);
            SaveLoad(inp.Get());
        }

        TDomain Get(const void* buf, size_t len, bool checkErr = true) const {
            TItem item;
            ui32 bid = BaseHash(buf, len, Seed_, NumBins_, Displ_.size(), &item);
            ui32 displ = Displ_[bid];
            TDomain pos = GetBinPos(NumBins_, item.First, item.Second, displ % NumBins_, displ / NumBins_);
            if (BinRank_.Size())
                pos -= BinRank_.Get(pos);
            if (pos >= Size())
                return npos;
            if (checkErr && FprSize_ && Fpr_.Size()) {
                if (Fpr_[pos] != (item.Fpr & MaskLowerBits(FprSize_))) {
                    return npos;
                }
            }
            return pos;
        }

        ui64 Space() const {
            return Displ_.Space() + BinRank_.Space() + Fpr_.Space();
        }

        ui32 NumBins() const {
            return NumBins_;
        }

        size_t MaxRank() const {
            return BinRank_.Size();
        }

        ui32 Size() const {
            return NumBins() - MaxRank();
        }

        ui8 ErrorBits() const {
            return FprSize_;
        }

        ui32 Seed() const {
            return Seed_;
        }

        template <class S>
        void SaveLoad(S* out) {
            NMinHash::TChdMinHashFuncHdr hdr(TChdMinHashFunc::Type);
            ::SaveLoad(out, hdr);
            hdr.Verify(TChdMinHashFunc::Type);
            ::SaveLoad(out, NumBins_);
            ::SaveLoad(out, Seed_);
            ::SaveLoad(out, Displ_);
            ::SaveLoad(out, BinRank_);
            ::SaveLoad(out, FprSize_);
            ::SaveLoad(out, Fpr_);
        }

    private:
        ui32 NumBins_;
        ui32 Seed_;
        ui8 FprSize_;
        TDispl Displ_;
        TRank BinRank_;
        TFpr Fpr_;
        friend class TChdHashBuilder;
    };

    class TDistChdMinHashFunc : TNonCopyable {
    public:
        typedef TStringBuf TKey;
        typedef ui64 TDomain;
        typedef TEliasFanoMonotoneArray<ui64> TOff;

        static int Type;
        static const TDomain npos = static_cast<TDomain>(-1);

        static ui32 Bucket(const void* buf, size_t len, ui32 numPortions) {
            return CityHash64((const char*)buf, len) % numPortions;
        }

        TDistChdMinHashFunc() {
        }

        explicit TDistChdMinHashFunc(IInputStream* inp) {
            Load(inp);
        }

        explicit TDistChdMinHashFunc(const TString& fileName) {
            TAutoPtr<IInputStream> inp = OpenInput(fileName);
            Load(inp.Get());
        }

        TDomain Get(const void* buf, size_t len, bool checkErr = true) const {
            Y_ASSERT(Buckets_.size());
            if (Buckets_.size() == 1) {
                const auto pos = Buckets_[0]->Get(buf, len, checkErr);
                if (pos == TChdMinHashFunc::npos) {
                    return TDistChdMinHashFunc::npos;
                }
                return pos;
            }
            ui32 off = Bucket(buf, len, Buckets_.size());
            const auto pos = Buckets_[off]->Get(buf, len, checkErr);
            if (pos == TChdMinHashFunc::npos) {
                return TDistChdMinHashFunc::npos;
            }
            return pos + Off_[off];
        }

        void Save(IOutputStream* s) const {
            TChdMinHashFuncHdr hdr(TDistChdMinHashFunc::Type);
            ::Save(s, hdr);
            hdr.Verify(TDistChdMinHashFunc::Type);
            ::SaveSize(s, Buckets_.size());
            for (size_t i = 0; i < Buckets_.size(); ++i) {
                ::Save(s, *(Buckets_[i]));
            }
            ::Save(s, Off_);
        }

        void Load(IInputStream* s) {
            TChdMinHashFuncHdr hdr(TDistChdMinHashFunc::Type);
            ::Load(s, hdr);
            hdr.Verify(TDistChdMinHashFunc::Type);
            size_t n = ::LoadSize(s);
            for (size_t i = 0; i < n; ++i) {
                Buckets_.push_back(new TChdMinHashFunc(s));
            }
            ::Load(s, Off_);
        }

        ui64 Space() const {
            ui64 res = Off_.Space();
            for (size_t i = 0; i < Buckets_.size(); ++i) {
                res += Buckets_[i]->Space();
            }
            return res;
        }

        ui32 Size() const {
            ui64 res = 0;
            for (size_t i = 0; i < Buckets_.size(); ++i) {
                res += Buckets_[i]->Size();
            }
            return res;
        }

    private:
        TVector<TAutoPtr<TChdMinHashFunc>> Buckets_;
        TOff Off_;
    };

}
