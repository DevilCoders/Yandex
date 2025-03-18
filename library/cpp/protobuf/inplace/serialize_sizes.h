#pragma once

#include <util/generic/bitops.h>
#include <util/generic/strbuf.h>

namespace NInPlaceProto {

    class TFieldSizes {
    private:
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetVarintSize64(ui64 value) noexcept {
            return (GetValueBitCount(value | 0x1) * 9 + 64) / 64;
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetVarintSize32(ui32 value) noexcept {
            return (GetValueBitCount(value | 0x1) * 9 + 64) / 64;
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetTagSize(ui32 fieldNumber) noexcept {
            return (GetValueBitCount(fieldNumber) * 9 + 91) / 64;
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE ui32 ZigZagEncode32(i32 n) noexcept {
            return (static_cast<ui32>(n) << 1) ^ static_cast<ui32>(n >> 31);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE ui64 ZigZagEncode64(i64 n) {
            return (static_cast<ui64>(n) << 1) ^ static_cast<ui64>(n >> 63);
        }

    public:
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetDouble(ui32 fieldNumber, double) noexcept {
            return GetTagSize(fieldNumber) + sizeof(double);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetFloat(ui32 fieldNumber, float) noexcept {
            return GetTagSize(fieldNumber) + sizeof(float);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetInt32(ui32 fieldNumber, i32 value) noexcept {
            return GetTagSize(fieldNumber) + (value < 0 ? 10 : GetVarintSize32(value));
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetInt64(ui32 fieldNumber, i64 value) noexcept {
            return GetTagSize(fieldNumber) + (value < 0 ? 10 : GetVarintSize64(value));
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetUInt32(ui32 fieldNumber, ui32 value) noexcept {
            return GetTagSize(fieldNumber) + GetVarintSize32(value);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetUInt64(ui32 fieldNumber, ui64 value) noexcept {
            return GetTagSize(fieldNumber) + GetVarintSize64(value);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetSInt32(ui32 fieldNumber, i32 value) noexcept {
            return GetTagSize(fieldNumber) + GetVarintSize32(ZigZagEncode32(value));
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetSInt64(ui32 fieldNumber, i64 value) noexcept {
            return GetTagSize(fieldNumber) + GetVarintSize64(ZigZagEncode64(value));
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetFixed32(ui32 fieldNumber, ui32) noexcept {
            return GetTagSize(fieldNumber) + sizeof(ui32);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetFixed64(ui32 fieldNumber, ui64) noexcept {
            return GetTagSize(fieldNumber) + sizeof(ui64);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetSFixed32(ui32 fieldNumber, i32) noexcept {
            return GetTagSize(fieldNumber) + sizeof(i32);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetSFixed64(ui32 fieldNumber, i64) noexcept {
            return GetTagSize(fieldNumber) + sizeof(i64);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetBool(ui32 fieldNumber, bool) noexcept {
            return GetTagSize(fieldNumber) + 1;
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetString(ui32 fieldNumber, size_t size) noexcept {
            return GetTagSize(fieldNumber) + GetVarintSize64(size) + size;
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetString(ui32 fieldNumber, TStringBuf value) noexcept {
            return GetString(fieldNumber, value.size());
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetBytes(ui32 fieldNumber, size_t size) noexcept {
            return GetString(fieldNumber, size);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetBytes(ui32 fieldNumber, TStringBuf value) noexcept {
            return GetString(fieldNumber, value);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetMessage(ui32 fieldNumber, size_t size) noexcept {
            return GetTagSize(fieldNumber) + GetVarintSize64(size) + size;
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetMessage(ui32 fieldNumber, TStringBuf value) noexcept {
            return GetMessage(fieldNumber, value.size());
        }

        Y_PURE_FUNCTION static Y_FORCE_INLINE ssize_t UpdateUInt32(ui32 oldValue, ui32 newValue) noexcept {
            return (ssize_t)GetVarintSize32(newValue) - (ssize_t)GetVarintSize32(oldValue);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE ssize_t UpdateUInt64(ui64 oldValue, ui64 newValue) noexcept {
            return (ssize_t)GetVarintSize64(newValue) - (ssize_t)GetVarintSize64(oldValue);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE ssize_t UpdateString(size_t oldSize, size_t newSize) noexcept {
            return (ssize_t)(GetVarintSize64(newSize) + newSize) - (ssize_t)(GetVarintSize64(oldSize) + oldSize);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE ssize_t UpdateBytes(size_t oldSize, size_t newSize) noexcept {
            return UpdateString(oldSize, newSize);
        }
        Y_PURE_FUNCTION static Y_FORCE_INLINE ssize_t UpdateMessage(size_t oldSize, size_t newSize) noexcept {
            return (ssize_t)(GetVarintSize64(newSize) + newSize) - (ssize_t)(GetVarintSize64(oldSize) + oldSize);
        }

        Y_PURE_FUNCTION static Y_FORCE_INLINE size_t GetMessageHeader(ui32 fieldNumber, size_t size) noexcept {
            return GetTagSize(fieldNumber) + GetVarintSize64(size);
        }
    };

} // namespace NInPlaceProto
