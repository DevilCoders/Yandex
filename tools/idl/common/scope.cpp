#include <yandex/maps/idl/scope.h>

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/utils/common.h>

#include <boost/algorithm/string.hpp>

namespace yandex {
namespace maps {
namespace idl {

Scope::Scope(std::initializer_list<const char*> items)
{
    value_.reserve(items.size());

    for (auto item : items) {
        value_.emplace_back(item);
    }
}

Scope::Scope(const std::string& str, char delimiter)
{
    if (!str.empty()) {
        boost::algorithm::split(
            value_, str, boost::is_any_of(std::string(1, delimiter)));
    }
}

std::string Scope::asString(const std::string& delimiter) const
{
    return boost::algorithm::join(value_, delimiter);
}

std::string Scope::asPrefix(const std::string& delimiter) const
{
    if (value_.empty()) {
        return "";
    } else {
        return asString(delimiter) + delimiter;
    }
}

const std::string& Scope::operator[](std::size_t i) const
{
    REQUIRE(i < size(),
        "Trying to read out-of-bounds scope item: " << i << " from " << size());
    return value_[i];
}
std::string& Scope::operator[](std::size_t i)
{
    REQUIRE(i < size(),
        "Trying to access out-of-bounds scope item: " << i << " from " << size());
    return value_[i];
}

const std::string& Scope::first() const
{
    REQUIRE(size() > 0, "Trying to read first item of an empty scope");
    return value_.front();
}
const std::string& Scope::last() const
{
    REQUIRE(size() > 0, "Trying to read last item of an empty scope");
    return value_[value_.size() - 1];
}

Scope Scope::subScope(std::size_t start, std::size_t end) const
{
    REQUIRE(start <= end,
        "Trying to create sub-scope with start > end: " << start << " > " << end);
    REQUIRE(end <= size(),
        "Trying to create sub-scope with end > size: " << end << " > " << size());

    if (start == end) {
        return Scope();
    } else {
        return Items{ value_.begin() + start, value_.begin() + end };
    }
}

Scope Scope::capitalized() const
{
    Items items;
    items.reserve(value_.size());

    for (const auto& item : value_) {
        items.push_back(utils::capitalizeWord(item));
    }
    return { std::move(items) };
}

Scope Scope::camelCased(bool capitalizeFirstSymbol) const
{
    Items items;
    items.reserve(value_.size());

    for (const auto& item : value_) {
        items.push_back(utils::toCamelCase(item, capitalizeFirstSymbol));
    }
    return { std::move(items) };
}

Scope& Scope::operator+=(const std::string& item)
{
    value_.push_back(item);
    return *this;
}
Scope& Scope::operator+=(const Scope& other)
{
    value_.reserve(size() + other.size());
    value_.insert(value_.end(), other.value_.begin(), other.value_.end());
    return *this;
}

Scope& Scope::operator--()
{
    REQUIRE(size() > 0, "Trying to remove last item of an empty scope");
    value_.pop_back();
    return *this;
}

Scope operator+(const Scope& left, const Scope& right)
{
    Scope result = left;
    result += right;
    return result;
}

Scope operator+(const Scope& scope, const std::string& item)
{
    return scope + Scope(item);
}
Scope operator+(const std::string& item, const Scope& scope)
{
    return Scope(item) + scope;
}

std::ostream& operator<<(std::ostream& out, const Scope& scope)
{
    out << std::string(scope);
    return out;
}

} // namespace idl
} // namespace maps
} // namespace yandex
