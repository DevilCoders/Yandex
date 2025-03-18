#pragma once

#include <array>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/string.h>
#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/system/unaligned_mem.h>

namespace NOffroad {
    template <class KeyData, class Serializer>
    class TFatSearcher {
    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TKeyData = KeyData;

        TFatSearcher() {
            Reset(TArrayRef<const char>(), TArrayRef<const char>());
        }

        TFatSearcher(const TArrayRef<const char>& fat, const TArrayRef<const char>& fatsub) {
            Reset(fat, fatsub);
        }

        TFatSearcher(const TBlob& fat, const TBlob& fatsub) {
            Reset(fat, fatsub);
        }

        void Reset(const TArrayRef<const char>& fat, const TArrayRef<const char>& fatsub) {
            Reset(TBlob::NoCopy(fat.data(), fat.size()), TBlob::NoCopy(fatsub.data(), fatsub.size()));
        }

        void Reset(const TBlob& fat, const TBlob& fatsub) {
            if (fatsub.Size() % sizeof(ui32) != 0) {
                Size_ = 0;
                ythrow yexception() << "Invalid fat subindex size.";
            }

            Fat_ = fat;
            FatSub_ = fatsub;
            Size_ = FatSub_.Size() / sizeof(ui32);

            RebuildLookupTable();
        }
        size_t Size() const {
            return Size_;
        }

        TKeyRef ReadKey(size_t index) const {
            return KeyAt(index);
        }

        TKeyData ReadData(size_t index) const {
            TKeyData result;
            Serializer::Deserialize(DataPtrAt(index), &result);
            return result;
        }

        size_t LowerBound(const TKeyRef& prefix) const {
            Y_ASSERT(!prefix.empty());

            auto cmp = [&](size_t l, const TStringBuf& r) {
                return KeyAt(l) < r;
            };

            size_t c = static_cast<ui8>(prefix[0]);
            auto range = xrange(Size_);
            return *::LowerBound(range.begin() + FirstBlockByChar_[c], range.begin() + FirstBlockByChar_[c + 1], prefix, cmp);
        }

    private:
        void RebuildLookupTable() {
            size_t firstChar = 0;
            for (size_t i = 1; i < Size_; i++) { /* Skip first dummy record */
                char c = KeyAt(i)[0];
                for (; firstChar <= static_cast<unsigned char>(c); firstChar++)
                    FirstBlockByChar_[firstChar] = i;
            }

            for (; firstChar <= 256; firstChar++)
                FirstBlockByChar_[firstChar] = Size_;
        }

        TStringBuf KeyAt(size_t index) const {
            const void* block = BlockAt(index);
            return TStringBuf(static_cast<const char*>(block) + 2, ReadUnaligned<ui16>(block));
        }

        const ui8* DataPtrAt(size_t index) const {
            const void* block = BlockAt(index);
            return static_cast<const ui8*>(block) + 2 + ReadUnaligned<ui16>(block);
        }

        const void* BlockAt(size_t index) const {
            Y_ASSERT(index < Size_);

            ui32 offset = ReadUnaligned<ui32>(FatSub_.AsCharPtr() + index * sizeof(ui32));
            return Fat_.AsCharPtr() + offset;
        }

    private:
        size_t Size_;
        TBlob Fat_;
        TBlob FatSub_;
        std::array<ui32, 257> FirstBlockByChar_;
    };

}
