#pragma once

#include <util/generic/string.h>
#include <util/generic/yexception.h>

class TSerializedString {
    TString String;
    const char* Pointer; // if Pointer is NULL base class TString is used
    size_t Length;

public:
    TSerializedString()
        : String()
        , Pointer(nullptr)
        , Length(0)
    {
    }
    explicit TSerializedString(const char* p, size_t n)
        : String()
        , Pointer(p)
        , Length(p ? n : 0) // s can be NULL
    {
        Y_ASSERT(!Pointer || !Pointer[Length]); // p must be NULL or point to null-terminated string
    }
    explicit TSerializedString(const TString& s)
        : String(s)
        , Pointer(nullptr)
        , Length(0)
    {
    }
    TSerializedString(const TSerializedString& other)
        : String(other.String)
        , Pointer(other.Pointer)
        , Length(other.Length)
    {
    }
    TSerializedString& operator=(const TSerializedString& other) {
        String = other.String;
        Pointer = other.Pointer;
        Length = other.Length;
        return *this;
    }
    operator const TString&() const {
        if (Pointer)
            ythrow yexception() << "can't return reference to TString";
        return String;
    }
    TStringBuf StrBuf() const {
        return Pointer ? TStringBuf(Pointer, Length) : TStringBuf(String);
    }
    const char* c_str() const {
        return (Pointer ? Pointer : String.c_str());
    }
    size_t size() const {
        return (Pointer ? Length : String.size());
    }
    const char* operator~() const {
        return c_str();
    }
    size_t operator+() const {
        return size();
    }
};
