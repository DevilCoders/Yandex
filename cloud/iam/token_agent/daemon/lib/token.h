#pragma once

#include <util/datetime/base.h>

#include <utility>

namespace NTokenAgent {
    class TToken {
    public:
        TToken() = default;

        TToken(std::string value, const TInstant& expiresAt)
            : Value(std::move(value))
            , ExpiresAt(expiresAt)
        {
        }

        [[nodiscard]] const std::string& GetValue() const {
            return Value;
        }

        [[nodiscard]] const TInstant& GetExpiresAt() const {
            return ExpiresAt;
        }

        [[nodiscard]] bool IsEmpty() const {
            return Value.empty();
        }

    private:
        std::string Value;
        TInstant ExpiresAt;
    };
}
