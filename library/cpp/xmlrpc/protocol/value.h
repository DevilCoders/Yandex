#pragma once

#include "errors.h"

#include <util/generic/ptr.h>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/typetraits.h>
#include <util/datetime/base.h>

#include <typeinfo>

class IInputStream;
class IOutputStream;

namespace NXmlRPC {
    struct IValue: public TThrRefBase {
        IValue();
        ~IValue() override;

        virtual bool IsA(const std::type_info& ti) const = 0;
        virtual void* Ptr() const = 0;

        virtual TStringBuf AsBlob() const = 0;
        virtual bool AsBool() const = 0;
        virtual TInstant AsDateTime() const = 0;
        virtual double AsDouble() const = 0;
        virtual i64 AsInteger() const = 0;
        virtual TString AsString() const = 0;
        virtual size_t ElementCount() const = 0;

        virtual void SerializeXml(IOutputStream& out) const = 0;
    };

    typedef TIntrusivePtr<IValue> IValueRef;

    IValue* Null() noexcept;

    template <class T>
    inline IValueRef Construct(const T& t) {
        extern IValue* ConstructXmlRPCValue(const T& t);

        return ConstructXmlRPCValue(t);
    }

    inline IValueRef Construct(char* v) {
        return Construct(TStringBuf(v));
    }

    inline IValueRef Construct(const char* v) {
        return Construct(TStringBuf(v));
    }

    template <class T>
    T DoCast(const IValue* v);

    struct TArray;
    struct TStruct;

    class TValue {
    public:
        inline TValue() noexcept
            : V_(Null())
        {
        }

        inline TValue(const IValueRef& v)
            : V_(v)
        {
        }

        template <class T>
        inline TValue(const T& t)
            : V_(Construct(t))
        {
        }

        inline bool IsNull() const noexcept {
            return V_.Get() == Null();
        }

        inline void Swap(TValue& v) noexcept {
            V_.Swap(v.V_);
        }

        void SerializeXml(IOutputStream& out) const;

        //assume array value
        const TValue& operator[](size_t n) const;
        TValue& operator[](size_t n);

        //assume struct value
        const TValue& operator[](const TStringBuf& key) const;
        TValue& operator[](const TStringBuf& key);

        //assume array or struct
        inline size_t Size() const {
            return V_->ElementCount();
        }

        //return value as struct
        inline TStruct& Struct() {
            return Cast<TStruct>();
        }

        inline const TStruct& Struct() const {
            return Cast<TStruct>();
        }

        //return value as array
        inline TArray& Array() {
            return Cast<TArray>();
        }

        inline const TArray& Array() const {
            return Cast<TArray>();
        }

        inline const IValue* Ptr() const noexcept {
            return V_.Get();
        }

    private:
        template <class T>
        inline bool IsA() const noexcept {
            return V_->IsA(typeid(T));
        }

        template <class T>
        inline const T& Cast() const {
            return const_cast<TValue*>(this)->Cast<T>();
        }

        template <class T>
        inline T& Cast() {
            if (IsA<T>()) {
                return *(T*)V_->Ptr();
            }

            ythrow TTypeMismatch() << "type mismatch";
        }

    private:
        IValueRef V_;
    };

    struct TArray: public TDeque<TValue> {
        inline TArray() {
        }

        template <typename... R>
        inline TArray(const R&... r) {
            PushMany(r...);
        }

        inline TArray& PushBack(const TValue& v) {
            push_back(v);

            return *this;
        }

        inline void PushMany() {
        }

        template <typename... R>
        inline void PushMany(const TValue& v, const R&... r) {
            PushBack(v).PushMany(r...);
        }

        const TValue& IndexOrNull(size_t n) const noexcept;
    };

    struct TStruct: public THashMap<TString, TValue> {
        inline TStruct& Set(const TString& k, const TValue& v) {
            (*this)[k] = v;

            return *this;
        }

        const TValue& Find(const TStringBuf& key) const;
    };

    template <class T>
    inline T Cast(const TValue& v) {
        return DoCast<T>(v.Ptr());
    }

    //some helpers
    TValue ParseValue(IInputStream& in);
    TValue ParseValue(const TString& data);
}
