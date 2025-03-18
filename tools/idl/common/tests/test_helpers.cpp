#include "test_helpers.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {

nodes::EnumField makeEnumFieldWithValue(
    std::string name,
    std::optional<std::string> value)
{
    return {
        { }, // doc
        name,
        value
    };
}

nodes::EnumField makeEnumField(std::string name)
{
    return makeEnumFieldWithValue(name, std::nullopt);
}

nodes::Enum makeEnum(
    std::string name,
    std::vector<std::string> fieldNames)
{
    return nodes::Enum {
        { }, // doc
        { }, // customCodeLink
        false, // isBitField
        {
            std::move(name),
            { } // target-specific names
        },
        { }, // based on ...proto...
        utils::convert(fieldNames, makeEnumField)
    };
}

} // namespace idl
} // namespace maps
} // namespace yandex
