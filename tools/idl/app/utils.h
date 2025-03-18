#pragma once

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/generator/output_file.h>

#include <boost/program_options.hpp>

#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace app {

// Special arguments
const char* const HELP = "help";
const char* const DRY_RUN = "dry-run";
const char* const REPORT_IMPORTS = "report-imports";
const char* const INPUT_FILE = "input-file";

// Generation arguments
const char* const IN_PROTO_ROOT = "in-proto-root";

const char* const IN_FRAMEWORK_SEARCH_PATH = "in-framework-search-path";
const char* const IN_FRAMEWORK_SEARCH_PATH_FULL = "in-framework-search-path,F";
const char* const IN_IDL_SEARCH_PATH = "in-idl-search-path";
const char* const IN_IDL_SEARCH_PATH_FULL = "in-idl-search-path,I";

const char* const BASE_PROTO_PACKAGE = "base-proto-package";
const char* const OUT_BASE_ROOT = "out-base-root";
const char* const OUT_ANDROID_ROOT = "out-android-root";
const char* const OUT_IOS_ROOT = "out-ios-root";

const char* const DISABLE_INTERNAL_CHECKS = "disable-internal-checks";
const char* const PUBLIC = "public";

const char* const USE_STD_OPTIONAL = "use-std-optional";

// Deprecated arguments
const char* const NO_USAGE_INFO = "no-usage-info";
const char* const IN_FRAMEWORK_PATH = "in-framework-path";
const char* const IN_IDL_PATH = "in-idl-path";
const char* const OUT_PUBLIC_HEADERS_ROOT = "out-public-headers-root";
const char* const OUT_CPP_IMPL_ROOT = "out-cpp-impl-root";
const char* const OUT_ANDROID_IMPL_ROOT = "out-android-impl-root";
const char* const OUT_IOS_IMPL_ROOT = "out-ios-impl-root";

/**
 * Extracts app name from cmd line - boost::program_options doesn't do that.
 */
std::string extractAppName(const char** argv);

/**
 * Builds usage heading for the app - first line in command line description.
 */
std::string buildUsageHeading(const std::string& appName);

/**
 * Builds command line arguments description for --help.
 */
boost::program_options::options_description buildHelpDesc();

/**
 * Builds command line arguments description for generating bindings.
 */
boost::program_options::options_description buildGenerationDesc();

Config extractConfig(const boost::program_options::variables_map& vm);
std::vector<std::string> extractInputFilePaths(
    const boost::program_options::variables_map& vm);

} // namespace app
} // namespace idl
} // namespace maps
} // namespace yandex
