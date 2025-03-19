#pragma once

#include <util/generic/variant.h>

#include <util/generic/overloaded.h>

namespace NDoom::NItemStorage {

namespace NPrivate {

struct TValueTag {};
struct TErrorTag {};

} // namespace NPrivate

template <typename T, typename E>
class TResult {
public:
    TResult(NPrivate::TValueTag, T&& value)
        : Sum_{ std::move(value) }
    {}

    TResult(NPrivate::TErrorTag, E&& error)
        : Sum_{ std::move(error) }
    {}

    bool IsOk() const {
        return std::holds_alternative<T>(Sum_);
    }

    bool IsError() const {
        return std::holds_alternative<E>(Sum_);
    }

    TLoadError* GetValue() const {
        return std::get_if<T>(Sum_);
    }

    TLoadError* GetError() const {
        return std::get_if<E>(Sum_);
    }

    const T& GetOrThrow() const {
        return std::visit(TOverloaded{
            [](const T& value) -> const T&  {
                return value;
            },
            [](const E& error) -> const T& {
                throw error;
            }
        }, Sum_);
    }

    T& GetOrThrow() {
        return std::visit(TOverloaded{
            [](T& value) -> T&  {
                return value;
            },
            [](E& error) -> T& {
                throw error;
            }
        }, Sum_);
    }

private:
    std::variant<T, E> Sum_;
};

template <typename T, typename E>
static TResult<T, E> MakeError(E error) {
    return { NPrivate::TErrorTag{}, std::move(error) };
}

template <typename T, typename E>
static TResult<T, E> MakeOk(T value) {
    return { NPrivate::TValueTag{}, std::move(value) };
}

} // namespace NDoom::NItemStorage
