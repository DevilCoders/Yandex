#pragma once

#include "impl/tags.h"
#include "writer.h"

#include <contrib/libs/mms/impl/container.h>
#include <contrib/libs/mms/version.h>

#include <util/generic/maybe.h>
#include <util/generic/yexception.h>

namespace NMms {
    template <class P, class T, class Policy = NMaybe::TPolicyUndefinedExcept>
    class TMaybe;

    template <class T, class Policy>
    class TMaybe<TMmapped, T, Policy>: public mms::impl::Offset {
    public:
        TMaybe(const TMaybe& c)
            : mms::impl::Offset(c)
        {
            if (c.offset_ == 0) {
                offset_ = 0;
            }
        }

        TMaybe& operator=(const TMaybe& c) {
            mms::impl::Offset::operator=(c);
            if (c.offset_ == 0) {
                offset_ = 0;
            }
            return *this;
        }

        bool Defined() const noexcept {
            return offset_ != 0;
        }
        bool Empty() const noexcept {
            return !Defined();
        }
        void CheckDefined() const {
            if (!Defined()) {
                Policy::OnEmpty(typeid(T));
            }
        }
        const T* Get() const noexcept {
            return Defined() ? Data() : nullptr;
        }
        const T& GetRef() const {
            CheckDefined();
            return *Data();
        }

        const T& operator*() const {
            return GetRef();
        }

        const T* operator->() const {
            return &GetRef();
        }

        const T& GetOrElse(const T& elseValue) const {
            return Defined() ? *Data() : elseValue;
        }

        const TMaybe& OrElse(const TMaybe& elseValue) const noexcept {
            return Defined() ? *this : elseValue;
        }

        bool operator==(const TMaybe& c) const {
            return Defined() == c.Defined() &&
                   (!Defined() || *Data() == *c.Data());
        }

        bool operator!=(const TMaybe& c) const {
            return !(*this == c);
        }

        explicit operator bool() const noexcept {
            return Defined();
        }

        static mms::FormatVersion formatVersion(mms::Versions& vs) {
            return vs.dependent<T>("optional");
        }

        using MmappedType = TMaybe<TMmapped, T, Policy>;

    private:
        const T* Data() const noexcept {
            return ptr<T>();
        }
    };

    template <class T, class Policy>
    class TMaybe<TStandalone, T, Policy>: public ::TMaybe<T, Policy> {
    private:
        using TBase = typename ::TMaybe<T, Policy>;

    public:
        using MmappedType = TMaybe<TMmapped, typename mms::impl::MmappedType<T>::type, Policy>;

        TMaybe()
            : TBase()
        {
        }
        TMaybe(const T& value)
            : TBase(value)
        {
        }
        TMaybe(const MmappedType& t)
            : TBase()
        {
            *this = t;
        }

        TMaybe& operator=(const T& value) {
            TBase::operator=(value);
            return *this;
        }
        TMaybe& operator=(const MmappedType& value) {
            if (value.Defined()) {
                TBase::operator=(T(*value));
            } else {
                TBase::Clear();
            }
            return *this;
        }

        template <class TWriter>
        size_t writeData(TWriter& w) const {
            return TBase::Defined() ? mms::impl::write(w, TBase::GetRef()) : mms::impl::nullOfs();
        }
        template <class TWriter>
        size_t writeField(TWriter& w, size_t pos) const {
            return mms::impl::writeOffset(w, pos);
        }
    };

}
