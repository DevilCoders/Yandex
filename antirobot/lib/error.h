#pragma once

#include "exceptions.h"

#include <util/generic/yexception.h>

#include <variant>

namespace NAntiRobot {
    class TError {
        using TException = std::variant<
            TForwardingFailure,
            TTimeoutException,
            THttpCodeException,
            TCaptchaGenerationError,
            TNehQueueOverflowException,
            yexception,
            std::exception>;

    public:
        TError() = default;

        template <class... Args>
        TError(Args&&... args)
            : Exception(new TException(std::forward<Args>(args)...))
        {
        }

        bool Defined() const noexcept {
            return static_cast<bool>(Exception);
        };

        std::exception& operator*() noexcept {
            Y_ASSERT(Exception);

            const auto& visitor = [](auto& e) -> decltype(auto) {
                return static_cast<std::exception&>(e);
            };
            return std::visit(visitor, *Exception);
        }

        const std::exception& operator*() const noexcept {
            Y_ASSERT(Exception);

            const auto& visitor = [](auto& e) -> decltype(auto) {
                return static_cast<const std::exception&>(e);
            };
            return std::visit(visitor, *Exception);
        }

        std::exception* operator->() noexcept {
            Y_ASSERT(Exception);

            const auto& visitor = [](auto&& e) -> decltype(auto) {
                return static_cast<std::exception&>(e);
            };
            return &std::visit(visitor, *Exception);
        }

        const std::exception* operator->() const noexcept {
            Y_ASSERT(Exception);

            const auto& visitor = [](auto& e) -> decltype(auto) {
                return static_cast<const std::exception&>(e);
            };
            return &std::visit(visitor, *Exception);
        }

        void Throw() {
            Y_ASSERT(Exception);

            const auto& visitor = [](auto e) {
                throw std::move(e);
            };
            std::visit(visitor, std::move(*Exception));
        }

        template<class T>
        bool Is() const {
            if (!Defined()) {
                return false;
            }

            return std::visit([] (const auto& exc) {
                return std::is_same_v<std::decay_t<decltype(exc)>, T>;
            }, *Exception);
        }

    private:
        THolder<TException> Exception;
    };

    template <class T>
    class TErrorOr: public TMoveOnly {
    public:
        using TValue = T;
        static_assert(!std::is_same_v<T, TError>);
        static_assert(!::TIsSpecializationOf<TErrorOr, T>::value);

        TErrorOr(TError&& error) noexcept
            : Value(std::in_place_type<TError>, std::move(error))
        {
        }

        TErrorOr(T&& value) noexcept {
            Value.template emplace<T>(value);
        }

        bool HasError() const noexcept {
            return std::holds_alternative<TError>(Value);
        };

        [[nodiscard]] TError ReleaseError() const noexcept {
            TError ret;
            const auto& visitor = [&](auto& v) {
                using type = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<type, TError>) {
                    ret = std::move(v);
                }
            };
            std::visit(visitor, Value);
            return ret;
        };

        [[nodiscard]] TError PutValueTo(T& to) const noexcept {
            TError ret;
            const auto& visitor = [&](auto& v) {
                using type = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<type, TError>) {
                    ret = std::move(v);
                } else if constexpr (std::is_same_v<type, T>) {
                    to = std::move(v);
                }
            };
            std::visit(visitor, Value);
            return ret;
        };

    private:
        mutable std::variant<TError, T> Value;
    };
}
