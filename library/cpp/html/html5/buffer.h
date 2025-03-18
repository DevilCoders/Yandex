#pragma once

#include "util.h"

#include <algorithm>
#include <assert.h>
#include <new>
#include <memory.h>

namespace NHtml5 {
    template <typename TInput>
    class TCodepointBuffer {
    public:
        inline TCodepointBuffer(const size_t initial = 1024)
            : Data_(new char[initial])
            , Length_(0)
            , Capacity_(initial)
        {
            assert(initial > 0);
        }

        inline ~TCodepointBuffer() {
            delete[] Data_;
        }

        inline const char* Data() const {
            return Data_;
        }

        inline size_t Length() const {
            return Length_;
        }

        inline void AppendChar(char c) {
            if (Length_ + 1 >= Capacity_) { // condition is correct
                Resize(Capacity_ * 2);
            }
            Data_[Length_++] = c;
        }

        inline void AppendString(const char* str) {
            const size_t len = strlen(str);
            if (Length_ + len >= Capacity_) {
                Resize(std::max(Capacity_ * 2, Length_ + len));
            }
            memcpy(Data_ + Length_, str, len);
            Length_ += len;
        }

        inline void AppendString(const TStringPiece& str) {
            if (Length_ + str.Length >= Capacity_) {
                Resize(std::max(Capacity_ * 2, Length_ + str.Length));
            }
            memcpy(Data_ + Length_, str.Data, str.Length);
            Length_ += str.Length;
        }

        //inline void Append(int c) {
        //    //MaybeResize(len);
        //    memcpy(Data_ + Length_, data, len);
        //    Length_ += len;
        //    // Length_ += TInput::Write(c, Data_);
        //}

        inline void Reset() {
            Length_ = 0;
        }

    private:
        inline void Resize(const size_t capacity) {
            char* new_data = new char[capacity];
            memcpy(new_data, Data_, Length_);
            delete[] Data_;
            Data_ = new_data;
            Capacity_ = capacity;
        }

    private:
        char* Data_;
        size_t Length_;
        size_t Capacity_;
    };

}
