#pragma once

#include "serialized.h"

#include <util/system/byteorder.h>

namespace NInPlaceProto {

    class TRegionDataProvider {
    private:
        const ui8* Start = nullptr;
        const ui8*const End = nullptr;
        bool Corrupted = false;

    public:
        template <typename TProtoMessage>
        explicit TRegionDataProvider(TSerialized<TProtoMessage> serialized)
            : Start(serialized.Data())
            , End(Start + serialized.Size())
        {
        }
        TRegionDataProvider(const ui8* start, size_t len)
            : Start(start)
            , End(Start + len)
        {
        }
        TRegionDataProvider(const char* start, size_t len)
            : Start((const ui8*)start)
            , End(Start + len)
        {
        }
        TRegionDataProvider(const ui8* start, const ui8* end)
            : Start(start)
            , End(end)
        {
        }
        TRegionDataProvider(const char* start, const char* end)
            : Start((const ui8*)start)
            , End((const ui8*)end)
        {
        }

        bool IsCorrupted() const noexcept {
            return Corrupted;
        }
        void SetCorrupted() noexcept {
            Corrupted = true;
        }

        bool NotEmpty() const noexcept {
            return Start < End;
        }

        void SkipVarint64() noexcept {
            int count = 0;
            ui32 b;
            do {
                if (count == 10 || Start >= End) {
                    Corrupted = true;
                    return;
                }
                b = *Start++;
            } while (b >= 0x80);
        }
        ui32 ReadVarint32Slow(ui32 firstByte) noexcept;
        inline ui32 ReadVarint32() noexcept {
            if (Start < End) {
                ui32 res = *Start;
                ++Start;
                if (res < 0x80) {
                    return res;
                }
                return ReadVarint32Slow(res);
            }
            Corrupted = true;
            return 0;
        }
        inline ui32 ReadTag() noexcept {
            const bool corrupted = Corrupted;
            const ui8*const start = Start;
            const ui8*const end = End;
            if (Y_LIKELY(!corrupted)) {
                if (Y_LIKELY(start < end)) {
                    ui32 res = *start;
                    ++Start;
                    if (res < 0x80) {
                        return res;
                    }
                    return ReadVarint32Slow(res);
                }
            }
            return 0;
        }
        ui64 ReadVarint64Slow() noexcept;
        ui64 ReadVarint64() noexcept {
            if (Y_LIKELY(Start < End)) {
                ui32 res = *Start;
                if (Y_LIKELY(res < 0x80)) {
                    ++Start;
                    return res;
                }
                return ReadVarint64Slow();
            }
            Corrupted = true;
            return 0;
        }
        ui32 ReadLittleEndian32() noexcept {
            if (Y_LIKELY(Start + sizeof(ui32) <= End)) {
                ui32 value = LittleToHost(*(const ui32*)Start);
                Start += sizeof(ui32);
                return value;
            }
            Corrupted = true;
            return 0;
        }
        ui64 ReadLittleEndian64() noexcept {
            if (Y_LIKELY(Start + sizeof(ui64) <= End)) {
                ui64 value = LittleToHost(*(const ui64*)Start);
                Start += sizeof(ui64);
                return value;
            }
            Corrupted = true;
            return 0;
        }
        void Skip(ui32 length) noexcept {
            if (Y_LIKELY(Start + length <= End)) {
                Start += length;
            } else {
                Corrupted = true;
            }
        }
        TArrayRef<const char> GetRegion(ui32 length) noexcept {
            if (Y_LIKELY(Start + length <= End)) {
                const ui8*const oldStart = Start;
                Start += length;
                return TArrayRef<const char>((const char*)oldStart, (const char*)Start);
            }
            Corrupted = true;
            return TArrayRef<const char>();
        }
        // to track unknown fields (to pass them unchanged to somewhere else),
        // call GetCurrentPos() before ReadTag() and after Skip*()
        const ui8* GetCurrentPos() const noexcept {
            return Start;
        }
    };

} // namespace NInPlaceProto
