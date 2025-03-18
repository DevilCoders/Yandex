#include "utils.h"

#include <yandex/maps/idl/utils/errors.h>
#include <yandex/maps/idl/utils/exception.h>
#include <yandex/maps/idl/utils/paths.h>

#include <boost/algorithm/string.hpp>

#include <exception>

const char* metaInfo(); // Implemented in auto-generated file

namespace yandex {
namespace maps {
namespace idl {
namespace app {

namespace po = boost::program_options;

std::string extractAppName(const char** argv)
{
    REQUIRE(argv && argv[0],
        "Couldn't extract Idl compiler name from command line.");

    return utils::Path(argv[0]).fileName();
}

std::string buildUsageHeading(const std::string& appName)
{
    return "Usage: " + appName + " [options] .idl files...\n";
}

po::options_description buildHelpDesc()
{
    po::options_description desc(
       "Auto-bindings generator.\n\nHelp",
       120);
    desc.add_options()
        (HELP, "print this message")
        (DRY_RUN, "print files to be generated")
        (REPORT_IMPORTS, "report import/include files for generated files")
        (NO_USAGE_INFO,
            "[[ DEPRECATED ]] don't print cmd-line help message / usage info when errors occur");
    return desc;
}

po::options_description buildGenerationDesc()
{
    po::options_description desc("Generation", 120);
    desc.add_options()
        (IN_PROTO_ROOT,
            po::value<std::string>(),
            "directory to search for .proto files")
        (IN_FRAMEWORK_SEARCH_PATH_FULL,
            po::value<std::vector<std::string>>(),
            "adds directory to search for .framework files")
        (IN_IDL_SEARCH_PATH_FULL,
            po::value<std::vector<std::string>>(),
            "adds directory to search for .idl files")

        (BASE_PROTO_PACKAGE,
            po::value<std::string>()->default_value("yandex.maps.proto"),
            "C++ namespace (in dot-notation) where protobuf decoders will be generated")

        (OUT_BASE_ROOT,
            po::value<std::string>()->default_value(""),
            "directory to store platform-independent files (.h and .cpp for native API and protobuf decoders)")
        (OUT_ANDROID_ROOT,
            po::value<std::string>()->default_value(""),
            "directory to store Android API and binding files (.h, .cpp and .java)")
        (OUT_IOS_ROOT,
            po::value<std::string>()->default_value(""),
            "directory to store iOS API and binding files (.h, .m and .mm)")
        (DISABLE_INTERNAL_CHECKS,
            "disable checks for @internal tag")
        (PUBLIC,
            "for public release, don't generate classes with @internal tag")
        (USE_STD_OPTIONAL,
            "enable std::optional in generator");



    po::options_description deprecated("Deprecated but still working", 120);
    deprecated.add_options()
        (IN_FRAMEWORK_PATH,
            po::value<std::string>(),
            "[[ DEPRECATED ]] colon-separated list of directories to search for .framework files")
        (IN_IDL_PATH,
            po::value<std::string>(),
            "[[ DEPRECATED ]] colon-separated list of directories to search for .idl files")

        (OUT_PUBLIC_HEADERS_ROOT,
            po::value<std::string>(),
            "[[ DEPRECATED ]] directory to store all header files (will be publicly accessible)")
        (OUT_CPP_IMPL_ROOT,
            po::value<std::string>()->default_value(""),
            "[[ DEPRECATED ]] directory to store platform-independent C++ implementation files")
        (OUT_ANDROID_IMPL_ROOT,
            po::value<std::string>()->default_value(""),
            "[[ DEPRECATED ]] directory to store Android binding files (.java and .cpp)")
        (OUT_IOS_IMPL_ROOT,
            po::value<std::string>()->default_value(""),
            "[[ DEPRECATED ]] directory to store iOS binding files (.m and .mm)");
    desc.add(deprecated);

    return desc;
}

namespace {

/**
 * Extracts required command-line argument. Throws an exception if not found.
 */
template <typename Parameter = std::string>
const Parameter& extract(const po::variables_map& vm, const char* const name)
{
    try {
        return vm[name].as<Parameter>();
    } catch (const std::exception& e) {
        throw utils::UsageError() << "Couldn't get command-line argument '--" <<
            name << "': " << e.what();
    }
}

/**
 * Extracts optional command-line argument. Returns std::nullopt if not found.
 */
template <typename Parameter = std::string>
std::optional<Parameter> peek(
    const po::variables_map& vm,
    const char* const name)
{
    auto iterator = vm.find(name);
    if (iterator == vm.end()) {
        return std::nullopt;
    } else {
        return iterator->second.as<Parameter>();
    }
}

/**
 * Extracts optional command-line argument.
 * Returns type's default value if not found.
 */
template <typename Parameter = std::string>
Parameter peekOrDefault(const po::variables_map& vm, const char* const name)
{
    auto iterator = vm.find(name);
    if (iterator == vm.end()) {
        return { };
    } else {
        return iterator->second.as<Parameter>();
    }
}

std::string stringWithSuffix(
    const po::variables_map& vm,
    const char* const name,
    const char* suffix)
{
    auto parameter = peekOrDefault<std::string>(vm, name);
    if (!parameter.empty()) {
        parameter += suffix;
    }
    return parameter;
}

} // namespace

Config extractConfig(const po::variables_map& vm)
{
    auto frameworkSearchPaths =
        peekOrDefault<std::vector<std::string>>(vm, IN_FRAMEWORK_SEARCH_PATH);
    if (frameworkSearchPaths.empty()) {
        boost::split(frameworkSearchPaths,
            extract(vm, IN_FRAMEWORK_PATH), boost::is_any_of(":"));
    }

    auto idlSearchPaths =
        peekOrDefault<std::vector<std::string>>(vm, IN_IDL_SEARCH_PATH);
    if (idlSearchPaths.empty()) {
        boost::split(idlSearchPaths,
            extract(vm, IN_IDL_PATH), boost::is_any_of(":"));
    }

    if (static_cast<bool>(vm.count(DISABLE_INTERNAL_CHECKS)) && static_cast<bool>(vm.count(PUBLIC)))
        throw utils::UsageError() << "Do not use `disable-internal-checks` and `public` arguments together";

    auto publicHeadersRoot = peek(vm, OUT_PUBLIC_HEADERS_ROOT);
    if (publicHeadersRoot) {
        return {
            extract(vm, IN_PROTO_ROOT),
            { frameworkSearchPaths },
            { idlSearchPaths },
            extract(vm, BASE_PROTO_PACKAGE),
            *publicHeadersRoot,
            extract(vm, OUT_CPP_IMPL_ROOT),
            *publicHeadersRoot,
            extract(vm, OUT_ANDROID_IMPL_ROOT),
            *publicHeadersRoot,
            extract(vm, OUT_IOS_IMPL_ROOT),
            static_cast<bool>(vm.count(DISABLE_INTERNAL_CHECKS)),
            static_cast<bool>(vm.count(PUBLIC)),
            static_cast<bool>(vm.count(USE_STD_OPTIONAL))
        };
    } else {
        return {
            extract(vm, IN_PROTO_ROOT),
            { frameworkSearchPaths },
            { idlSearchPaths },
            extract(vm, BASE_PROTO_PACKAGE),
            stringWithSuffix(vm, OUT_BASE_ROOT, "/include"),
            stringWithSuffix(vm, OUT_BASE_ROOT, "/impl"),
            stringWithSuffix(vm, OUT_ANDROID_ROOT, "/include"),
            stringWithSuffix(vm, OUT_ANDROID_ROOT, "/impl"),
            stringWithSuffix(vm, OUT_IOS_ROOT, "/include"),
            stringWithSuffix(vm, OUT_IOS_ROOT, "/impl"),
            static_cast<bool>(vm.count(DISABLE_INTERNAL_CHECKS)),
            static_cast<bool>(vm.count(PUBLIC)),
            static_cast<bool>(vm.count(USE_STD_OPTIONAL))
        };
    }
}

std::vector<std::string> extractInputFilePaths(const po::variables_map& vm)
{
    return extract<std::vector<std::string>>(vm, INPUT_FILE);
}

} // namespace app
} // namespace idl
} // namespace maps
} // namespace yandex
