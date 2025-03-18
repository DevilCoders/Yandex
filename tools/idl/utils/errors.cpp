#include <yandex/maps/idl/utils/errors.h>

#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace utils {

GroupedError::GroupedError(
    const Path& relativePath,
    const std::string& description,
    const std::vector<std::string>& errors)
{
    *this << "Idl file " << asConsoleBold(relativePath.inQuotes()) <<
        "\n  " << description << ":\n";

    for (const auto& item : errors) {
        *this << "    " << item << '\n';
    }
}

} // namespace utils
} // namespace idl
} // namespace maps
} // namespace yandex
