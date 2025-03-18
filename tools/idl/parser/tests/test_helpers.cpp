#include "test_helpers.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/parser/parse_idl.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <memory>
#include <sstream>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

Environment simpleEnv(
    utils::SearchPaths&& frameworkPaths,
    utils::SearchPaths&& idlPaths,
    bool disableInternalChecks)
{
    return Environment({
        "", // inProtoRoot
        std::move(frameworkPaths),
        std::move(idlPaths),
        "", // baseProtoPackage
        "", // outBaseHeadersRoot
        "", // outBaseImplRoot
        "", // outAndroidHeadersRoot
        "", // outAndroidImplRoot
        "", // outIosHeadersRoot
        "",  // outIosImplRoot
        disableInternalChecks,
        false,
        false
    });
}

std::string generateIdlDoc(const nodes::DocBlock& docBlock)
{
    boost::format format(docBlock.format);
    for (const auto& link : docBlock.links) {
        if (link.memberName.empty()) {
            format % std::string(link.scope);
        } else {
            auto value = std::string(link.scope) + '#' + link.memberName;
            if (link.parameterTypeRefs) {
                std::vector<std::string> parameters;
                for (const auto& typeRef : *link.parameterTypeRefs) {
                    auto prefix = typeRef.isConst ? "const " : "";
                    parameters.push_back(prefix + typeRefToString(typeRef));
                }
                value += boost::algorithm::join(parameters, ", ");
            }
            format % value;
        }
    }
    return format.str();
}

void checkStringsEqualByLine(
    const std::string& actual,
    const std::string& expected)
{
    std::istringstream actualLines(actual), expectedLines(expected);
    std::string actualLine, expectedLine;

    std::size_t lineNumber = 1;
    while (actualLines && expectedLines) {
        std::getline(actualLines, actualLine);
        std::getline(expectedLines, expectedLine);

        BOOST_REQUIRE_MESSAGE(
            actualLine == expectedLine,
            "\nActual string\n[" + actual + "]\ndiffers from "
                "expected string\n[" + expected + "]\n on line #" +
                std::to_string(lineNumber) + ":\n[" + actualLine + "]");
        // BOOST_REQUIRE exits here in case of failure

        ++lineNumber;
    }

    if (!actualLines && !expectedLines) {
        return; // Both streams consumed - strings were equal
    }

    if (expectedLines) { // Expected is longer
        BOOST_FAIL("Actual string\n[" + actual + "]\ndoes not have the "
            "suffix\n[" + expectedLines.str().substr(expectedLines.tellg()) +
            "]\nof expected string\n[" + expected + "]");
    } else { // Actual is longer
        BOOST_FAIL("Expected string\n[" + expected + "]\ndoes not have the "
            "suffix\n[" + actualLines.str().substr(actualLines.tellg()) +
            "]\nof actual string\n[" + actual + "]");
    }
}

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
