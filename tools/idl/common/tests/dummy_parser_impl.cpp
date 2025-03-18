#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/parser/parse_framework.h>
#include <yandex/maps/idl/parser/parse_idl.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

// Following parse methods are not implemented in this module (common). They
// are left for "parser" implementing modules. But here, complete executable
// must be created to run tests, so we need to provide some implementation.

Framework parseFramework(
    const std::string& /* filePath */,
    const std::string& /* fileContents */)
{
    return { };
}

nodes::Root parseIdl(
    const std::string& /* filePath */,
    const std::string& /* fileContents */)
{
    return { };
}

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
