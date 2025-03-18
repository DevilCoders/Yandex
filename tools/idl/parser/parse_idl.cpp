#include <yandex/maps/idl/parser/parse_idl.h>

#include "bison.h"
#include "flex.h"
#include "internal/bison_helpers.h"

#include <yandex/maps/idl/utils/errors.h>
#include <yandex/maps/idl/utils/exception.h>

#include <memory>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

nodes::Root parseIdl(
    const std::string& filePath,
    const std::string& fileContents)
{
    std::istringstream fileContentsStream(fileContents);
    BisonedFlexLexer scanner(&fileContentsStream);

    std::unique_ptr<nodes::Root> root;
    Errors errors;
    yyparse(&scanner, root, errors);
    errors.handlePendingError();

    if (!errors.errors().empty()) {
        throw utils::GroupedError(
            filePath, "contains following syntax errors", errors.errors());
    }

    REQUIRE(root, "Couldn't parse file '" << filePath << "'");

    return std::move(*root);
}

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
