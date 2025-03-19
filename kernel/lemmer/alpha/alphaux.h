#pragma once

#include "abc.h"
#include <library/cpp/unicode/normalization/normalization.h>
#include <util/generic/ptr.h>

namespace NLemmerAux {
    template <size_t staticBufferSize>
    class TBuffer : TNonCopyable {
    private:
        wchar16 BufferStatic[staticBufferSize];
        THolder<wchar16, TDeleteArray> BufferDynamic;
        size_t BufferSize;
        wchar16* Buffer;

    public:
        TBuffer()
            : BufferSize(staticBufferSize)
            , Buffer(BufferStatic)
        {
        }
        wchar16* operator()(size_t size) {
            if (size > BufferSize) {
                size_t newBufferSize = BufferSize * 2;
                if (newBufferSize < size)
                    newBufferSize = size;
                THolder<wchar16> newBuffer(new wchar16[newBufferSize]);
                memcpy(newBuffer.Get(), Buffer, BufferSize * sizeof(wchar16));
                BufferDynamic.Reset(newBuffer.Release());
                BufferSize = newBufferSize;
                Buffer = BufferDynamic.Get();
            }
            return Buffer;
        }
        const wchar16* GetPtr() const {
            return Buffer;
        }
        size_t GetSize() const {
            return BufferSize;
        }
    };

    class TLimitedString {
    private:
        wchar16* const Buffer;
        const size_t MaxLength;
        size_t Len;
        bool Valid;

    public:
        TLimitedString(wchar16* buf, size_t maxLen)
            : Buffer(buf)
            , MaxLength(maxLen)
            , Len(0)
            , Valid(true)
        {
        }
        void push_back(wchar16 c) {
            if (Len < MaxLength)
                Buffer[Len++] = c;
            else
                Valid = false;
        }
        size_t Length() const {
            return Len;
        }
        const wchar16* GetStr() const {
            return Buffer;
        }
        bool IsValid() const {
            return Valid;
        }
    };

    template <class T>
    inline void Decompose(const wchar16* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFKD>(str, len, out);
    }

    // uses softer denormalization than Decompose method, for example \u00B9 does not decomposes to No, see http://www.unicode.org/reports/tr15/
    template <class T>
    inline void SoftDecompose(const wchar16* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFD>(str, len, out);
    }

    inline NLemmer::NDetail::TTransdRet SoftDecompose(const wchar16* str, size_t len, wchar16* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        SoftDecompose(str, len, out);
        return NLemmer::NDetail::TTransdRet(out.Length(), len, out.IsValid(), true);
    }

    inline NLemmer::NDetail::TTransdRet Decompose(const wchar16* str, size_t len, wchar16* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        Decompose(str, len, out);
        return NLemmer::NDetail::TTransdRet(out.Length(), len, out.IsValid(), true);
    }

    inline TUtf16String Decompose(const wchar16* str, size_t len) {
        TUtf16String out;
        out.reserve(len);
        Decompose(str, len, out);
        return out;
    }

    inline TUtf16String SoftDecompose(const wchar16* str, size_t len) {
        TUtf16String out;
        out.reserve(len);
        SoftDecompose(str, len, out);
        return out;
    }

    template <class T>
    inline void Compose(const wchar16* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFKC>(str, len, out);
    }

    // uses softer normalization than Compose method, see http://www.unicode.org/reports/tr15/
    template <class T>
    inline void SoftCompose(const wchar16* str, size_t len, T& out) {
        Normalize< ::NUnicode::NFC>(str, len, out);
    }

    inline NLemmer::NDetail::TTransdRet Compose(const wchar16* str, size_t len, wchar16* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        Compose(str, len, out);
        return NLemmer::NDetail::TTransdRet(out.Length(), len, out.IsValid(), true);
    }
    inline NLemmer::NDetail::TTransdRet SoftCompose(const wchar16* str, size_t len, wchar16* buf, size_t bufLen) {
        TLimitedString out(buf, bufLen);
        SoftCompose(str, len, out);
        return NLemmer::NDetail::TTransdRet(out.Length(), len, out.IsValid(), true);
    }
}
