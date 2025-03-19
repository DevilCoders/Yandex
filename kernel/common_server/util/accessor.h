#pragma once

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>

#define RTLINE_ACCEPTOR_VARIATIVE(ClassName, name, type) \
private:\
    bool Is ## name ## Initialized = false;\
    type name; \
public:\
    ClassName& Set ## name(const type& value) noexcept {\
        Is ## name ## Initialized = true;\
        name = value;\
        return *this;\
    }\
    ClassName& Set ## name(type&& value) noexcept {\
        Is ## name ## Initialized = true;\
        name = std::move(value);\
        return *this;\
    }\
    const type* Get ## name ## Ptr() const noexcept {\
        return Is ## name ## Initialized ? &name : nullptr;\
    }\
    bool Has ## name() const noexcept {\
        return Is ## name ## Initialized;\
    }\
    const type& Get ## name ## Safe() const noexcept {\
        CHECK_WITH_LOG(Is ## name ## Initialized); \
        return name; \
    }

#define RTLINE_FLAG_ACCEPTOR(ClassName, name, defaultValue) \
private:\
    bool name ## Flag = defaultValue;\
public:\
    ClassName& Set##name(const bool value) noexcept {\
        name ## Flag = value;\
        return *this;\
    }\
    bool Is ## name() const noexcept {\
        return name ## Flag;\
    }\
    ClassName& name(const bool value = true) noexcept {\
        name ## Flag = value;\
        return *this;\
    }\
    ClassName& Not ## name(const bool value = true) noexcept {\
        name ## Flag = !value; \
        return *this;\
    }

#define RTLINE_READONLY_FLAG_ACCEPTOR(name, defaultValue) \
private:\
    bool name ## Flag = defaultValue;\
public:\
    bool Is ## name() const noexcept {\
        return name ## Flag;\
    }

#define RTLINE_READONLY_ACCEPTOR_IMPL(name, type, declSpecifiers, defaultValue) \
private:\
    declSpecifiers type name = defaultValue;\
public:\
    const type& Get ## name() const noexcept {\
        return name;\
    }\
private:

#define RTLINE_ACCEPTOR_IMPL(ClassName, name, type, declSpecifiers, defaultValue, methodsModifier) \
RTLINE_READONLY_ACCEPTOR_IMPL(name, type, declSpecifiers, defaultValue)\
public:\
    ClassName& Set ## name(const type& value) methodsModifier {\
        name = value;\
        return *this;\
    }\
    ClassName& Set##name(type&& value) methodsModifier {\
        name = std::move(value);\
        return *this;\
    }\
    type& Mutable ## name() methodsModifier {\
        return name;\
    }\
    type&& DetachMutable ## name() methodsModifier {\
        return std::move(name);\
    }\
private:

#define RTLINE_ACCEPTOR(ClassName, name, type, defaultValue) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, private:, defaultValue, noexcept)
#define RTLINE_MUTABLE_ACCEPTOR(ClassName, name, type, defaultValue) RTLINE_ACCEPTOR_IMPL(const ClassName, name, type, mutable, defaultValue, const noexcept)
#define RTLINE_ACCEPTOR_DEF(ClassName, name, type) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, private:, type(), noexcept)
#define RTLINE_PROTECT_ACCEPTOR(ClassName, name, type, defaultValue) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, protected:, defaultValue, noexcept)
#define RTLINE_PROTECT_ACCEPTOR_DEF(ClassName, name, type) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, protected:, type(), noexcept)

#define RTLINE_CONST_ACCEPTOR(name, type) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, const, type())
#define RTLINE_READONLY_ACCEPTOR(name, type, defaultValue) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, private:, defaultValue)
#define RTLINE_READONLY_ACCEPTOR_DEF(name, type) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, private:, type())
#define RTLINE_READONLY_PROTECT_ACCEPTOR(name, type, defaultValue) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, protected:, defaultValue)
#define RTLINE_READONLY_PROTECT_ACCEPTOR_DEF(name, type) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, protected:, type())
#define RTLINE_READONLY_MUTABLE_ACCEPTOR(name, type, defaultValue) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, mutable, defaultValue)
#define RTLINE_READONLY_MUTABLE_ACCEPTOR_DEF(name, type) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, mutable, type())

#define CS_ACCESS(ClassName, type, name, defaultValue) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, private:, defaultValue, noexcept)
#define CSA_MUTABLE(ClassName, type, name, defaultValue) RTLINE_ACCEPTOR_IMPL(const ClassName, name, type, mutable, defaultValue, const noexcept)
#define CSA_MUTABLE_DEF(ClassName, type, name) RTLINE_ACCEPTOR_IMPL(const ClassName, name, type, mutable, type(), const noexcept)
#define CSA_DEFAULT(ClassName, type, name) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, private:, type(), noexcept)
#define CSA_PROTECTED(ClassName, type, name, defaultValue) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, protected:, defaultValue, noexcept)
#define CSA_PROTECTED_DEF(ClassName, type, name) RTLINE_ACCEPTOR_IMPL(ClassName, name, type, protected:, type(), noexcept)
#define CSA_FLAG(ClassName, name, defaultValue) RTLINE_FLAG_ACCEPTOR(ClassName, name, defaultValue)
#define CSA_READONLY_FLAG(name, defaultValue) RTLINE_READONLY_FLAG_ACCEPTOR(name, defaultValue)

#define CSA_CONST(type, name) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, const, type())
#define CSA_READONLY(type, name, defaultValue) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, private:, defaultValue)
#define CSA_READONLY_DEF(type, name) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, private:, type())
#define CSA_READONLY_PROTECTED(type, name, defaultValue) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, protected:, defaultValue)
#define CSA_READONLY_PROTECTED_DEF(type, name) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, protected:, type())
#define CSA_READONLY_MUTABLE(type, name, defaultValue) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, mutable, defaultValue)
#define CSA_READONLY_MUTABLE_DEF(type, name) RTLINE_READONLY_ACCEPTOR_IMPL(name, type, mutable, type())

#define RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, mType, declSpecifiers) \
private:\
    declSpecifiers mType<type> name;\
public:\
    bool Has##name() const noexcept {\
        return name.Defined();\
    }\
    const type& Get##name##Unsafe() const noexcept {\
        return *name;\
    }\
    type& Get##name##Unsafe() noexcept {\
        return *name;\
    }\
    const mType<type>& Get##name##Maybe() const noexcept {\
        return name;\
    }\
    template <class TMaybeExt = TMaybe<type>>\
    TMaybeExt Get##name##MaybeDetach() const noexcept {\
        if (name) {\
            return *name;\
        } else {\
            return Nothing();\
        }\
    }\
    type Get##name##Def(const type defValue) const noexcept {\
        return name.GetOrElse(defValue);\
    }\
    const type* Get##name##Safe() const noexcept {\
        return name.Get();\
    }\
private:

#define RTLINE_ACCEPTOR_MAYBE_IMPL(ClassName, name, mType, type, declSpecifiers) \
RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, mType, declSpecifiers)\
public:\
    ClassName& Set##name(const type& value) noexcept {\
        name = value;\
        return *this;\
    }\
    ClassName& Set##name(type&& value) noexcept {\
        name = std::move(value);\
        return *this;\
    }\
    template <class TMaybeExt = TMaybe<type>>\
    ClassName& Set##name(TMaybe<type>&& value) noexcept {\
        if (!!value) {\
            name = std::move(*value);\
        } else {\
            name = {};\
        }\
        return *this;\
    }\
    template <class TMaybeExt = TMaybe<type>>\
    ClassName& Set##name(const TMaybe<type>& value) noexcept {\
        if (!!value) {\
            name = *value;\
        } else {\
            name = {};\
        }\
        return *this;\
    }\
    ClassName& Drop##name() noexcept {\
        name = {};\
        return *this;\
    }\
    mType<type>& Mutable##name##Maybe() noexcept {\
        return name;\
    }\
private:

#define RTLINE_ACCEPTOR_MAYBE_MUTABLE_IMPL(ClassName, name, mType, type, declSpecifiers) \
RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, mType, declSpecifiers)\
public:\
    const ClassName& Set##name(const type& value) const noexcept {\
        name = value;\
        return *this;\
    }\
    const ClassName& Set##name(type&& value) const noexcept {\
        name = std::move(value);\
        return *this;\
    }\
    const ClassName& Drop##name() const noexcept {\
        name = {};\
        return *this;\
    }\
    mType<type>& Mutable##name##Maybe() const noexcept {\
        return name;\
    }\
private:

#define RTLINE_ACCEPTOR_MAYBE(ClassName, name, type) RTLINE_ACCEPTOR_MAYBE_IMPL(ClassName, name, TMaybeFail, type, private:)
#define RTLINE_ACCEPTOR_MAYBE_MUTABLE(ClassName, name, type) RTLINE_ACCEPTOR_MAYBE_MUTABLE_IMPL(ClassName, name, TMaybeFail, type, mutable)
#define RTLINE_ACCEPTOR_MAYBE_PROTECTED(ClassName, name, type) RTLINE_ACCEPTOR_MAYBE_IMPL(ClassName, name, TMaybeFail, type, protected:)
#define RTLINE_READONLY_ACCEPTOR_MAYBE_EXCEPT(name, type) RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, TMaybe, private:)
#define RTLINE_READONLY_ACCEPTOR_MAYBE(name, type) RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, TMaybeFail, private:)

#define CSA_MAYBE(ClassName, type, name) RTLINE_ACCEPTOR_MAYBE_IMPL(ClassName, name, TMaybeFail, type, private:)
#define CSA_MAYBE_MUTABLE(ClassName, type, name) RTLINE_ACCEPTOR_MAYBE_MUTABLE_IMPL(ClassName, name, TMaybeFail, type, mutable)
#define CSA_MAYBE_PROTECTED(ClassName, type, name) RTLINE_ACCEPTOR_MAYBE_IMPL(ClassName, name, TMaybeFail, type, protected:)
#define CSA_MAYBE_EXCEPT(ClassName, type, name) RTLINE_ACCEPTOR_MAYBE_IMPL(ClassName, name, TMaybe, type, private:)
#define CSA_MAYBE_PROTECTED_EXCEPT(ClassName, type, name) RTLINE_ACCEPTOR_MAYBE_IMPL(ClassName, name, TMaybe, type, protected:)
#define CSA_READONLY_MAYBE_EXCEPT(type, name) RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, TMaybe, private:)
#define CSA_READONLY_MAYBE_PROTECTED_EXCEPT(type, name) RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, TMaybe, protected:)
#define CSA_READONLY_MAYBE(type, name) RTLINE_READONLY_ACCEPTOR_MAYBE_IMPL(name, type, TMaybeFail, private:)

#define CS_HOLDER_IMPL(type, name)\
private:\
    THolder<type> name;\
public:\
    bool Has##name() const {\
        return name.Get();\
    }\
    const type& Get##name() const {\
        return *name;\
    }\
    type& Get##name() {\
        return *name;\
    }\
private:

#define CS_HOLDER(type, name) CS_HOLDER_IMPL(type, name)

#define CS_HOLDER_OVERRIDE_IMPL(type, name)\
private:\
    THolder<type> name;\
public:\
    bool Has##name() const override {\
        return name.Get();\
    }\
    const type& Get##name() const override {\
        return *name;\
    }\
private:

#define CS_HOLDER_OVERRIDE(type, name) CS_HOLDER_OVERRIDE_IMPL(type, name)
