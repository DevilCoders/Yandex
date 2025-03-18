#pragma once

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/root.h>

#include <boost/test/unit_test.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {

template <typename ...Nodes>
nodes::Root makeRoot(Nodes&&... nodes)
{
    return {
        { }, // objcInfix
        { }, // imports
        { std::move(nodes)... }
    };
}

nodes::EnumField makeEnumFieldWithValue(
    std::string name,
    std::optional<std::string> value);

nodes::EnumField makeEnumField(std::string name);

nodes::Enum makeEnum(
    std::string name,
    std::vector<std::string> fieldNames);

} // namespace idl
} // namespace maps
} // namespace yandex
