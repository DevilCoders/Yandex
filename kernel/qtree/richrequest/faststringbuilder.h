#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <library/cpp/charset/wide.h>

template <typename TDerived, typename TCharType>
class TFastStringBuilderBase {
    typedef TFastStringBuilderBase<TDerived, TCharType> TSelf;
public:
    typedef TBasicString<TCharType> TStringType;
    typedef TBasicStringBuf<TCharType> TStringBufType;

    inline TFastStringBuilderBase()
        : DefaultBuffer_()
        , Buffer_(DefaultBuffer_)
    {
    }

    inline TFastStringBuilderBase(TStringType& externalBuffer)
        : Buffer_(externalBuffer)
    {
    }

    inline const TStringType& Buffer() const {
        return Buffer_;
    }

    inline void Reset() {
        Buffer_.clear();
    }

    inline TDerived& operator << (TCharType str) {
        Append(str);
        return *This();
    }

    inline TDerived& operator << (const TStringBufType& str) {
        Append(str.data(), str.size());
        return *This();
    }

    inline TDerived& operator << (const TStringType& str) {
        Append(str.data(), str.size());
        return *This();
    }

    inline TDerived& operator << (const TCharType* str) {
        Append(str, std::char_traits<TCharType>::length(str));
        return *This();
    }

protected:
    inline void Append(const TCharType* str, size_t size) {
        Buffer_.append(str, size);
    }

    inline void Append(TCharType str) {
        Buffer_.append(str);
    }

    inline void Reserve(size_t extraSize) {
        Buffer_.reserve(Buffer_.size() + extraSize);
    }

private:
    inline TDerived* This() {
        return static_cast<TDerived*>(this);
    }

    TStringType DefaultBuffer_;
    TStringType& Buffer_;
};

namespace NFastStringBuilderPrivate {
    template <typename T, bool isnum = std::is_arithmetic<T>::value>
    struct TNumericAppender {
        template <typename TDst>
        static inline void AppendTo(TDst& dst, T num) {
            char buf[256];
            dst << TStringBuf(buf, ToString<T>(num, buf, sizeof(buf)));
        }
    };

    template <typename T>
    struct TNumericAppender<T, false> {
        // AppendTo() is not implemented to cause a compile time error
    };
}

class TFastStringBuilder: public TFastStringBuilderBase<TFastStringBuilder, char> {
    typedef TFastStringBuilderBase<TFastStringBuilder, char> TBase;
public:
    inline TFastStringBuilder(ECharset encoding = CODES_UNKNOWN)
        : Encoding(encoding)
    {
    }

    inline TFastStringBuilder(TString& externalBuffer, ECharset encoding = CODES_UNKNOWN)
        : TBase(externalBuffer)
        , Encoding(encoding)
    {
    }

    using TBase::operator << ;

    // auto-encode for wide strings, encoding should be specified on construction
    inline TFastStringBuilder& operator << (const TWtringBuf& str) {
        EncodeAppend(str);
        return *this;
    }

    template <typename T>
    inline TFastStringBuilder& operator << (const T& num) {
        typedef typename TTypeTraits<T>::TFuncParam TParam;
        NFastStringBuilderPrivate::TNumericAppender<TParam>::AppendTo(*this, num);
        return *this;
    }

private:
    inline void EncodeAppend(const TWtringBuf& str) {
        if (Y_UNLIKELY(Encoding == CODES_UNKNOWN))
            ythrow yexception() << "Cannot append wide string: encoding was not specified";
        TStringBuf encoded = WideToChar(str, EncodeBuffer, Encoding);
        TBase::Append(encoded.data(), encoded.size());
    }

    ECharset Encoding;
    TString EncodeBuffer;
};

class TFastWtringAsciiBuilder: public TFastStringBuilderBase<TFastWtringAsciiBuilder, wchar16> {
    typedef TFastStringBuilderBase<TFastWtringAsciiBuilder, wchar16> TBase;
public:
    inline TFastWtringAsciiBuilder() {
    }

    inline TFastWtringAsciiBuilder(TUtf16String& externalBuffer)
        : TBase(externalBuffer)
    {
    }

    // allow appending of ASCII chars directly
    inline TFastWtringAsciiBuilder& operator << (const char str) {
        AppendAscii(str);
        return *this;
    }

    inline TFastWtringAsciiBuilder& operator << (const char* str) {
        AppendAscii(str, std::char_traits<char>::length(str));
        return *this;
    }

    inline TFastWtringAsciiBuilder& operator << (const TStringBuf& str) {
        AppendAscii(str.data(), str.size());
        return *this;
    }

    using TBase::operator << ;

    template <typename T>
    inline TFastWtringAsciiBuilder& operator << (const T& num) {
        typedef typename TTypeTraits<T>::TFuncParam TParam;
        ::NFastStringBuilderPrivate::TNumericAppender<TParam>::AppendTo(*this, num);
        return *this;
    }

private:
    static inline bool IsAscii(const char chr) {
        return static_cast<unsigned char>(chr) < 0x80;
    }

    inline void AppendAscii(char chr) {
        Y_ASSERT(IsAscii(chr));
        TBase::Append(static_cast<wchar16>(chr));
    }

    inline void AppendAscii(const char* str, size_t size) {
        TBase::Reserve(size);
        for (const char* end = str + size; str != end; ++str)
            AppendAscii(*str);
    }
};

