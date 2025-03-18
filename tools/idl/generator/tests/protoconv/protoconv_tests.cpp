#include "tests/test_helpers.h"

#include "protoconv/common.h"
#include "protoconv/error_checker.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/generator/protoconv/generator.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

BOOST_AUTO_TEST_CASE(protoconv_tests)
{
    // Test working .idl files
    auto env = protoEnv("generator/tests/protoconv/idl_root");

    for (const auto& searchPath : env.config.inIdlSearchPaths) {
        for (const auto& idlPath : searchPath.childPaths(true, ".idl")) {
            auto idl = env.idl(idlPath);

            auto files = generate(idl);
            BOOST_CHECK_EQUAL(
                files[0].prefixPath, env.config.outBaseHeadersRoot);
            BOOST_CHECK_EQUAL(files[0].suffixPath, filePath(idl, true));

            BOOST_CHECK_EQUAL(
                files[1].prefixPath, env.config.outBaseImplRoot);
            BOOST_CHECK_EQUAL(files[1].suffixPath, filePath(idl, false));

            for (const auto& file : files) {
                BOOST_REQUIRE(!file.text.empty());
                file.fullPath().write(file.text);
            }
        }
    }

    // Test .idl file with Protobuf-related errors
    auto env2 = protoEnv("generator/tests/protoconv/idl_root_with_errors");
    auto idl = env2.idl("proto/with_protoconv_errors.idl");
    auto errors = ErrorChecker(idl).check();

    std::vector<std::string> expectedErrors = {
        "IncompleteKind: Idl enum doesn't have constants [AIRPORT, AREA, COUNTRY, DISTRICT, HOUSE, HYDRO, LOCALITY, METRO_STATION, OTHER, PROVINCE, RAILWAY_STATION, REGION, ROUTE, STATION, STREET, VEGETATION] from Protobuf enum. Please check that in .idl, your enum constants are in CamelCase, and in .proto - UPPER_CASE.",
        "NotFound: Protobuf type not found 'NotFound'",
        "Address.unknownField: Field not found in Protobuf message",
        "Address.optionalFormattedAddress: Field is 'optional', but in Protobuf it is 'required'",
        "Address.requiredPostalCode: Field is 'required', but in Protobuf it is 'optional' without default value",
        "Address.intAddress: Idl field's type is not based on Protobuf field's type",
        "Address.mapAddress: Type not allowed for field based on Protobuf field",
        "Address.kind: Protobuf type is message, but Idl's is neither struct nor variant",
        "Address.component: Idl field's type is not based on Protobuf field's type",
        "Component.address: Protobuf type is enum, but Idl's is not",
        "CustomHeaderMessage.repeatedS: Field is 'required', but in Protobuf it is 'optional' without default value",
        "CustomHeaderMessage.repeatedS: Protobuf type is not 'repeated'",
        "CustomHeaderMessage.notRepeated: Protobuf type is 'repeated'",
        "CustomHeaderMessage.vectorVector: 'vector<vector<...>>' is not supported for Protobuf-based fields",
    };

    BOOST_REQUIRE_EQUAL(errors.size(), expectedErrors.size());
    for (size_t i = 0; i < errors.size(); ++i) {
        BOOST_CHECK_EQUAL(errors[i], expectedErrors[i]);
    }
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
