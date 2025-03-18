#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/array_ref.h>
#include <util/system/unaligned_mem.h>

#include <library/cpp/vec4/vec4.h>

namespace NOffroad {
    class TBitInput: public TNonCopyable {
    public:
        TBitInput() {
            Reset(TArrayRef<const char>());
        }

        TBitInput(const TArrayRef<const char>& region) {
            Reset(region);
        }

        void Swap(TBitInput& other) {
            DoSwap(Current_, other.Current_);
            DoSwap(BitsAdvanced_, other.BitsAdvanced_);
            DoSwap(BitsProcessed_, other.BitsProcessed_);
            DoSwap(Start_, other.Start_);
            DoSwap(End_, other.End_);
            DoSwap(Pos_, other.Pos_);
        }

        void Reset(const TArrayRef<const char>& region) {
            Current_ = 0;
            BitsAdvanced_ = 0;
            BitsProcessed_ = 0;
            Start_ = reinterpret_cast<const ui8*>(region.data());
            End_ = Start_ + region.size();
            Pos_ = Start_;
            Seek(0);
        }

        bool Seek(ui64 bitPos) {
            size_t size = (End_ - Start_) * 8;
            if (bitPos > size) {
                return false;
            }
            if (bitPos == size) {
                BitsProcessed_ = 0;
                BitsAdvanced_ = 0;
                Pos_ = End_;
                return true;
            }
            BitsProcessed_ = bitPos % 8;
            Pos_ = Start_ + bitPos / 8;
            BitsAdvanced_ = Advance();
            Y_ASSERT(BitsProcessed_ < BitsAdvanced_);
            return true;
        }

        size_t Read(ui64* data, size_t bits) {
            Y_ASSERT(bits <= 64);

            const size_t newBits = BitsProcessed_ + bits;

            if (newBits >= BitsAdvanced_) {
                Y_ASSERT(BitsProcessed_ < 64);
                ui64 r = (Current_ >> BitsProcessed_);

                const size_t bitsRead = BitsAdvanced_ - BitsProcessed_;
                BitsAdvanced_ = Advance();
                if (bitsRead < 64) {
                    r |= (Current_ << bitsRead);
                }

                *data = r;
                bits = Min(bits - bitsRead, BitsAdvanced_);
                BitsProcessed_ = bits;
                return bitsRead + bits;
            } else {
                *data = Current_ >> BitsProcessed_;
                BitsProcessed_ = newBits;
                return bits;
            }
        }

        size_t Read(TVec4u* data, size_t bits) {
            size_t result = 0;

            if (bits > 16) {
                ui64 v0, v1;
                result += Read(&v0, bits * 2);
                result += Read(&v1, bits * 2);
                *data = BitLoad(v0, v1, bits);
            } else {
                ui64 v0;
                result += Read(&v0, bits * 4);
                *data = BitLoad(v0, v0 >> (bits * 2), bits);
            }

            return result;
        }

        ui64 Position() const {
            return (Pos_ - Start_) * 8 + BitsProcessed_ - BitsAdvanced_;
        }

    private:
        size_t Advance() {
            const ui8* ptr = Pos_;
            if (ptr + 8 > End_) {
                if (ptr >= End_)
                    return 0;

                const size_t bytes = End_ - ptr;
                memcpy(&Current_, ptr, bytes);
                Pos_ = End_;
                return bytes * 8;
            } else {
                Current_ = ReadUnaligned<ui64>(ptr);
                Pos_ += 8;
                return 64;
            }
        }

        static inline TVec4u BitLoad(ui64 value0, ui64 value1, size_t bits) {
            const __m128i a = _mm_unpacklo_epi64(_mm_cvtsi64_si128(value0), _mm_cvtsi64_si128(value1));
            const __m128i m32 = _mm_set_epi32(0xffffffff, 0, 0xffffffff, 0);
            const __m128i b = _mm_sll_epi64(a, _mm_cvtsi32_si128(32 - bits));
            return static_cast<__m128i>(_mm_or_si128(_mm_and_si128(m32, b), _mm_andnot_si128(m32, a)));
        }

    private:
        ui64 Current_;
        size_t BitsAdvanced_;
        size_t BitsProcessed_;
        const ui8* Start_;
        const ui8* End_;
        const ui8* Pos_;
    };

} //namespace NOffroad
