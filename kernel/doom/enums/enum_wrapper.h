#pragma once

#include <util/system/types.h>

#include <type_traits>

namespace NDoom {

namespace NPrivate {

template<class Enum, class Wrapper>
std::false_type IsWrapped(Enum, Wrapper);

template<class Enum, class Wrapper>
struct TIsWrapped:
    std::is_same<std::true_type, decltype(IsWrapped(std::declval<Enum>(), std::declval<Wrapper>()))> {
};

class TEnumWrapper {
public:
    TEnumWrapper(): Data_(0) {}

    explicit TEnumWrapper(ui32 value) {
        Data_ = value;
    }

    operator unsigned int() const {
        return Data_;
    }

    operator unsigned long() const {
        return Data_;
    }

    operator unsigned long long() const {
        return Data_;
    }

    operator int() const {
        return Data_;
    }

    operator long() const {
        return Data_;
    }

    operator long long() const {
        return Data_;
    }

protected:
    ui32 Data() const {
        return Data_;
    }

    void SetData(ui32 data) {
        Data_ = data;
    }

private:
    ui32 Data_;
};

} // namespace NPrivate

/**
 * Defines a new enum wrapper - a class that acts as an enumeration,
 * but can store values of different `enum`s.
 *
 * You can declare these `enum`s with subsequent invocations of
 * `Y_DECLARE_WRAPPED_ENUM`:
 *
 * \code
 * Y_DEFINE_ENUM_WRAPPER(EStreamType)
 * Y_DECLARE_WRAPPED_ENUM(EStreamTypeV1, EStreamType)
 * Y_DECLARE_WRAPPED_ENUM(EStreamTypeV2, EStreamType)
 * // Now EStreamType can store both EStreamTypeV1 and EStreamTypeV2.
 * \endcode
 *
 * Note that this is mainly useful for writing self-documented code and for type
 * safety (so that you don't have to use `uint`s in places where several
 * different enums must be accepted).
 */
#define Y_DEFINE_ENUM_WRAPPER(WRAPPER)                                          \
class WRAPPER: public ::NDoom::NPrivate::TEnumWrapper {                         \
public:                                                                         \
    WRAPPER() {}                                                                \
    explicit WRAPPER(ui32 value) { SetData(value); }                            \
    template<class Enum, class = std::enable_if_t<::NDoom::NPrivate::TIsWrapped<Enum, WRAPPER>::value>> \
    WRAPPER(Enum value) { SetData(value); }                                     \
    template<class Enum, class = std::enable_if_t<::NDoom::NPrivate::TIsWrapped<Enum, WRAPPER>::value>> \
    operator Enum() const { return static_cast<Enum>(Data()); }                 \
    friend bool operator==(WRAPPER l, WRAPPER r) { return static_cast<ui32>(l) == static_cast<ui32>(r); } \
    friend bool operator!=(WRAPPER l, WRAPPER r) { return static_cast<ui32>(l) != static_cast<ui32>(r); } \
    template<class Enum, class = std::enable_if_t<::NDoom::NPrivate::TIsWrapped<Enum, WRAPPER>::value>> \
    friend bool operator==(const WRAPPER& l, Enum r) { return static_cast<Enum>(l) == r; } \
    template<class Enum, class = std::enable_if_t<::NDoom::NPrivate::TIsWrapped<Enum, WRAPPER>::value>> \
    friend bool operator==(Enum l, const WRAPPER& r) { return l == static_cast<Enum>(r); } \
    template<class Enum, class = std::enable_if_t<::NDoom::NPrivate::TIsWrapped<Enum, WRAPPER>::value>> \
    friend bool operator!=(const WRAPPER& l, Enum r) { return static_cast<Enum>(l) != r; } \
    template<class Enum, class = std::enable_if_t<::NDoom::NPrivate::TIsWrapped<Enum, WRAPPER>::value>> \
    friend bool operator!=(Enum l, const WRAPPER& r) { return l != static_cast<Enum>(r); } \
};

/**
 * \see Y_DEFINE_ENUM_WRAPPER
 */
#define Y_DECLARE_WRAPPED_ENUM(ENUM, WRAPPER)                                   \
    std::true_type IsWrapped(ENUM, WRAPPER);


} // namespace NDoom
