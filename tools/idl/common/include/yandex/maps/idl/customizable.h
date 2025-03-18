#pragma once

#include <string>
#include <utility>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {

/**
 * Holds value specific to some target (language).
 */
template <typename Value>
struct TargetSpecificValue {
    std::string target;
    Value value;
};

/**
 * Holds value that can be customized with target languages.
 */
template <typename Value>
class CustomizableValue {
public:
    CustomizableValue() = default;
    CustomizableValue(
        Value&& original,
        std::vector<TargetSpecificValue<Value>>&& targetSpecificValues)
        : original_(std::move(original)),
          targetSpecificValues_(std::move(targetSpecificValues))
    {
    }

    /**
     * Returns default (original) value - used when no custom target is given.
     */
    const Value& original() const
    {
        return original_;
    }

    /**
     * Returns target-specific value based on given target, or the original
     * value if target was empty or no match was found.
     */
    const Value& operator[](const std::string& target) const
    {
        if (!target.empty()) {
            for (const auto& targetSpecificValue : targetSpecificValues_) {
                if (targetSpecificValue.target == target)
                    return targetSpecificValue.value;
            }
        }
        return original_;
    }

    /**
     * Tells if a target-specific override is defined.
     */
    bool isDefined(const std::string& target) const
    {
        for (const auto& targetSpecificValue : targetSpecificValues_) {
            if (targetSpecificValue.target == target)
                return true;
        }

        return false;
    }

private:
    Value original_;
    std::vector<TargetSpecificValue<Value>> targetSpecificValues_;
};

} // namespace idl
} // namespace maps
} // namespace yandex
