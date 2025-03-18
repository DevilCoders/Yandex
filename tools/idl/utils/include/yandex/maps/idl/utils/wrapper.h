#pragma once

#include <utility>

namespace yandex {
namespace maps {
namespace idl {
namespace utils {

/**
 * Base class for wrapper-like entities.
 *
 * To avoid copying all these methods.
 */
template <typename Value>
class Wrapper {
public:
    Wrapper() { }
    Wrapper(const Wrapper& copy) : value_(copy.value_) { }
    Wrapper(Wrapper&& source) : value_(std::move(source.value_)) { }

    Wrapper(Value value) : value_(std::move(value)) { }

    operator Value() const { return value_; }

    Wrapper& operator=(const Wrapper& /* copy */) = default;
    Wrapper& operator=(Wrapper&& /* source */) = default;

    bool operator==(const Wrapper& other) const { return value_ == other.value_; }
    bool operator!=(const Wrapper& other) const { return value_ != other.value_; }

    bool operator==(const Value& value) const { return value_ == value; }
    bool operator!=(const Value& value) const { return value_ != value; }

    typename Value::iterator begin() { return value_.begin(); }
    typename Value::iterator end() { return value_.end(); }
    typename Value::const_iterator begin() const { return value_.begin(); }
    typename Value::const_iterator end() const { return value_.end(); }

protected:
    using Wrapper_ = Wrapper; // Allows non-template access from sub-classes
    using Value_ = Value;

    Value value_;
};

} // namespace utils
} // namespace idl
} // namespace maps
} // namespace yandex
