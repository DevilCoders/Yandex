#include "rankselect.h"

#include "blob_reader.h"

#include <library/cpp/pop_count/popcount.h>
#include <library/cpp/select_in_word/select_in_word.h>

#include <util/generic/bitops.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

#include <memory>

namespace NSuccinctArrays {
    TRank::~TRank() {
        if (OwnsData_) {
            delete[] Counts_;
        }
    }

    TRank::TRank()
        : NumWords_(0)
        , NumCounts_(0)
        , NumOnes_(0)
        , Counts_(nullptr)
        , OwnsData_(true)
    {
    }

    TRank::TRank(const ui64* bits, ui64 numBits)
        : NumWords_(((numBits ? numBits : 1) + 63) / 64)
        , NumCounts_((((numBits ? numBits : 1) + 64 * 8 - 1) / (64 * 8)) * 2)
        , NumOnes_(0)
        , Counts_(nullptr)
        , OwnsData_(true)
    {
        Y_ASSERT(bits);
        Counts_ = new ui64[(size_t)(NumCounts_ + 1)]();
        NumOnes_ = 0;
        size_t pos = 0;
        for (ui64 i = 0; i < NumWords_; i += 8, pos += 2) {
            Counts_[pos] = NumOnes_;
            NumOnes_ += PopCount(bits[i]);
            for (int j = 1; j < 8; j++) {
                Counts_[pos + 1] |= (NumOnes_ - Counts_[pos]) << 9 * (j - 1);
                if (i + j < NumWords_)
                    NumOnes_ += PopCount(bits[i + j]);
            }
        }
        Counts_[NumCounts_] = NumOnes_;
    }

    TRank::TRank(TRank&& other)
        : NumWords_(other.NumWords_)
        , NumCounts_(other.NumCounts_)
        , NumOnes_(other.NumOnes_)
        , Counts_(other.Counts_)
        , OwnsData_(other.OwnsData_)
    {
        other.NumWords_ = 0;
        other.NumCounts_ = 0;
        other.NumOnes_ = 0;
        other.Counts_ = nullptr;
        other.OwnsData_ = true;
    }

    TRank& TRank::operator=(TRank&& other) {
        NumWords_ = other.NumWords_;
        NumCounts_ = other.NumCounts_;
        NumOnes_ = other.NumOnes_;
        if (OwnsData_) {
            delete[] Counts_;
        }
        Counts_ = other.Counts_;
        OwnsData_ = other.OwnsData_;

        other.NumWords_ = 0;
        other.NumCounts_ = 0;
        other.NumOnes_ = 0;
        other.Counts_ = nullptr;
        other.OwnsData_ = true;

        return *this;
    }

    ui64 TRank::Rank(const ui64* bits, ui64 k) const {
        Y_ASSERT(bits);
        if (k >= (NumWords_ * 64))
            return NumOnes_;
        ui64 word = k / 64;
        ui64 block = word / 4 & ~1;
        uint32_t offset = (word - 1) & 7;
        Y_ASSERT((block + 1) <= NumCounts_);
        return Counts_[block] +
               ((Counts_[block + 1] >> (offset * 9)) & 0x1FF) +
               PopCount(bits[word] & ((ULL(1) << (k % 64)) - 1));
    }

    void TRank::Save(IOutputStream* out) const {
        ::Save(out, NumWords_);
        ::Save(out, NumCounts_);
        ::Save(out, NumOnes_);
        ::SavePodArray(out, Counts_, (size_t)(NumCounts_ + 1));
    }

    void TRank::SaveForReadonlyAccess(IOutputStream* out) const {
        // All data is already aligned; just call regular Save().
        this->Save(out);
    }

    void TRank::Load(IInputStream* inp) {
        ::Load(inp, NumWords_);
        ::Load(inp, NumCounts_);
        ::Load(inp, NumOnes_);
        if (OwnsData_) {
            delete[] Counts_;
        }
        Counts_ = new ui64[(size_t)(NumCounts_ + 1)];
        OwnsData_ = true;
        ::LoadPodArray(inp, Counts_, (size_t)(NumCounts_ + 1));
    }

    TBlob TRank::LoadFromBlob(const TBlob& blob) {
        TBlobReader reader(blob);
        reader.ReadInteger(&NumWords_);
        reader.ReadInteger(&NumCounts_);
        reader.ReadInteger(&NumOnes_);
        if (OwnsData_) {
            delete[] Counts_;
        }
        Counts_ = const_cast<ui64*>(reader.ReadArray<ui64>(NumCounts_ + 1));
        OwnsData_ = false;
        return reader.Tail();
    }

    ui64 TRank::Space() const {
        return CHAR_BIT * (sizeof(*Counts_) * (NumCounts_ + 1) +
                           sizeof(NumWords_) +
                           sizeof(NumCounts_) +
                           sizeof(NumOnes_));
    }

    TRankSelect::~TRankSelect() {
        if (OwnsData_) {
            delete[] Inventory_;
        }
    }

    TRankSelect::TRankSelect()
        : InventorySize_(0)
        , OnesPerInventory_(0)
        , Log2OnesPerInventory_(0)
        , Inventory_(nullptr)
        , OwnsData_(true)
    {
    }

    TRankSelect::TRankSelect(const ui64* bits, ui64 numBits)
        : InventorySize_(0)
        , OnesPerInventory_(0)
        , Log2OnesPerInventory_(0)
        , Inventory_(nullptr)
        , Rank_(bits, numBits)
        , OwnsData_(true)
    {
        Y_ASSERT(bits);
        Log2OnesPerInventory_ = numBits ? (size_t)MostSignificantBit((Rank_.NumOnes_ * 16 * 64 + numBits - 1) / numBits) : 0;
        OnesPerInventory_ = ULL(1) << Log2OnesPerInventory_;
        InventorySize_ = (size_t)(Rank_.NumOnes_ + OnesPerInventory_ - 1) / OnesPerInventory_;
        Inventory_ = new ui32[InventorySize_ + 1]();
        ui64 d = 0;
        ui64 mask = OnesPerInventory_ - 1;
        for (size_t i = 0; i < Rank_.NumWords_; i++) {
            for (int j = 0; j < 64; j++) {
                if (bits[i] & 1ULL << j) {
                    if ((d & mask) == 0) {
                        Inventory_[(size_t)(d >> Log2OnesPerInventory_)] = (i / 8) * 2;
                    }
                    d++;
                }
            }
        }
        Inventory_[InventorySize_] = (Rank_.NumWords_ / 8) * 2;
    }

    TRankSelect::TRankSelect(TRankSelect&& other)
        : InventorySize_(other.InventorySize_)
        , OnesPerInventory_(other.OnesPerInventory_)
        , Log2OnesPerInventory_(other.Log2OnesPerInventory_)
        , Inventory_(other.Inventory_)
        , Rank_(std::move(other.Rank_))
        , OwnsData_(other.OwnsData_)
    {
        other.InventorySize_ = 0;
        other.OnesPerInventory_ = 0;
        other.Log2OnesPerInventory_ = 0;
        other.Inventory_ = nullptr;
        other.OwnsData_ = true;
    }

    TRankSelect& TRankSelect::operator=(TRankSelect&& other) {
        InventorySize_ = other.InventorySize_;
        OnesPerInventory_ = other.OnesPerInventory_;
        Log2OnesPerInventory_ = other.Log2OnesPerInventory_;
        if (OwnsData_) {
            delete[] Inventory_;
        }
        Inventory_ = other.Inventory_;
        OwnsData_ = other.OwnsData_;
        Rank_ = std::move(other.Rank_);

        other.InventorySize_ = 0;
        other.OnesPerInventory_ = 0;
        other.Log2OnesPerInventory_ = 0;
        other.Inventory_ = nullptr;
        other.OwnsData_ = true;

        return *this;
    }

    ui64 TRankSelect::Rank(const ui64* bits, ui64 k) const {
        return Rank_.Rank(bits, k);
    }

    ui64 TRankSelect::Select(const ui64* bits, ui64 rank) const {
        Y_ASSERT(bits);
        if (rank >= Rank_.NumOnes_)
            return npos;
        size_t inventoryIndexLeft = (size_t)(rank >> Log2OnesPerInventory_);
        size_t blockLeft = Inventory_[inventoryIndexLeft];
        size_t blockRight = Inventory_[inventoryIndexLeft + 1];
        if (rank >= Rank_.Counts_[blockRight]) {
            blockRight = (blockLeft = blockRight) + 2;
        } else {
            size_t blockMiddle;
            while (blockRight - blockLeft > 2) {
                blockMiddle = (blockRight + blockLeft) / 2 & ~1;
                if (rank >= Rank_.Counts_[blockMiddle])
                    blockLeft = blockMiddle;
                else
                    blockRight = blockMiddle;
            }
        }
        const ui64 rankInBlock = rank - Rank_.Counts_[blockLeft];
        const ui64 rankInBlockStep9 = rankInBlock * ONES_STEP_9;
        const ui64 subcounts = Rank_.Counts_[blockLeft + 1];
        const ui64 offsetInBlock = (ULEQ_STEP_9(subcounts, rankInBlockStep9) * ONES_STEP_9 >> 54 & 0x7);
        const ui64 word = blockLeft * 4 + offsetInBlock;
        const ui64 rankInWord = rankInBlock - (subcounts >> ((offsetInBlock - 1) & 7) * 9 & 0x1FF);
        return word * 64 + SelectInWord(bits[(size_t)word], (size_t)rankInWord);
    }

    void TRankSelect::Save(IOutputStream* out) const {
        this->Save(out, false);
    }

    void TRankSelect::SaveForReadonlyAccess(IOutputStream* out) const {
        this->Save(out, true);
    }

    void TRankSelect::Load(IInputStream* inp) {
        ::Load(inp, InventorySize_);
        ::Load(inp, OnesPerInventory_);
        ::Load(inp, Log2OnesPerInventory_);
        if (OwnsData_) {
            delete[] Inventory_;
        }
        Inventory_ = new ui32[InventorySize_ + 1];
        OwnsData_ = true;
        ::LoadPodArray(inp, Inventory_, InventorySize_ + 1);
        ::Load(inp, Rank_);
    }

    TBlob TRankSelect::LoadFromBlob(const TBlob& blob) {
        TBlobReader reader(blob);
        reader.ReadInteger(&InventorySize_);
        reader.ReadInteger(&OnesPerInventory_);
        reader.ReadInteger(&Log2OnesPerInventory_);
        if (OwnsData_) {
            delete[] Inventory_;
        }
        Inventory_ = const_cast<ui32*>(reader.ReadArray<ui32>(InventorySize_ + 1));
        OwnsData_ = false;
        if (InventorySize_ % 2 == 0) {
            ui32 padding{};
            reader.ReadInteger(&padding);
        }
        return Rank_.LoadFromBlob(reader.Tail());
    }

    ui64 TRankSelect::Space() const {
        return Rank_.Space() + CHAR_BIT * (sizeof(*Inventory_) * (InventorySize_ + 1) +
                                           sizeof(InventorySize_) +
                                           sizeof(OnesPerInventory_) +
                                           sizeof(Log2OnesPerInventory_));
    }

    void TRankSelect::Save(IOutputStream* out, bool aligned) const {
        ::Save(out, InventorySize_);
        ::Save(out, OnesPerInventory_);
        ::Save(out, Log2OnesPerInventory_);
        ::SavePodArray(out, Inventory_, InventorySize_ + 1);
        if (aligned && (InventorySize_ % 2 == 0)) {
            ui32 padding{};
            ::Save(out, padding);
        }
        if (aligned) {
            Rank_.SaveForReadonlyAccess(out);
        } else {
            ::Save(out, Rank_);
        }
    }

}
