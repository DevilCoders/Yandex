#pragma once

#include <util/generic/noncopyable.h>
#include <util/stream/output.h>
#include <util/stream/null.h>
#include <util/system/yassert.h>

#include <library/cpp/offroad/utility/masks.h>
#include <library/cpp/vec4/vec4.h>

namespace NOffroad {
    class TBitOutput: public TNonCopyable {
    public:
        TBitOutput() {
            Reset(&Cnull);
        }

        TBitOutput(IOutputStream* stream) {
            Reset(stream);
        }

        ~TBitOutput() {
            Finish();
        }

        void Reset(IOutputStream* stream) {
            Stream_ = stream;
            Current_ = 0;
            Bits_ = 0;
            Position_ = 0;
            IsFinished_ = false;
        }

        void Write(ui64 value, size_t bits) {
            Y_ASSERT(!IsFinished_);
            Y_ASSERT(bits <= 64);
            Y_ASSERT((value & ~ScalarMask(bits)) == 0);

            size_t newBits = Bits_ + bits;
            Y_ASSERT(Bits_ < 64);
            Current_ = Current_ | (value << Bits_);

            if (newBits >= 64) {
                Position_ += 64;
                Stream_->Write(&Current_, sizeof(Current_));
                newBits -= 64;
                Current_ = (Bits_ == 0) ? 0 : (value >> (64 - Bits_));
            }

            Bits_ = newBits;
        }

        void Write(const TVec4u& value, size_t bits) {
            Write(value.Value<0>(), bits);
            Write(value.Value<1>(), bits);
            Write(value.Value<2>(), bits);
            Write(value.Value<3>(), bits);
        }

        void Finish() {
            if (IsFinished_)
                return;

            /* Works for little/big endianness. */
            for (size_t i = 0; i < Bits_; i += 8) {
                ui8 symbol = Current_;
                Stream_->Write(&symbol, 1);
                Current_ = Current_ >> 8;
            }

            IsFinished_ = true;
        }

        ui64 Position() const {
            return Position_ + Bits_;
        }

        bool IsFinished() const {
            return IsFinished_;
        }

    private:
        ui64 Current_ = 0;
        ui64 Bits_ = 0;
        IOutputStream* Stream_ = nullptr;
        ui64 Position_ = 0;
        bool IsFinished_ = false;
    };

}
