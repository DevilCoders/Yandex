#pragma once

#include <yandex/maps/idl/utils/exception.h>
#include <yandex/maps/idl/utils/paths.h>

#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace utils {

/**
 * Represents errors in application usage. E.g. invalid command-line option
 * was given, .framework / .idl / .proto file was not found...
 *
 * These exceptions should be caught in main(...) function, and reported with
 * some helpful message. All other exceptions are considered app's internal
 * errors and should not be caught - this will make sure that when debugging
 * the error, you can easily find the exact place where it occurred.
 */
class UsageError : public Exception {
};

/**
 * Groups multiple usage errors of similar types (e.g. multiple syntax
 * errors), to easily show all of them at once.
 */
class GroupedError : public UsageError {
public:
    /**
     * Groups multiple errors of similar types.
     *
     * @param relativePath - relative file path where errors occurred
     * @param description - describes the type of errors
     * @param errors
     */
    GroupedError(
        const Path& relativePath,
        const std::string& description,
        const std::vector<std::string>& errors);
};

} // namespace utils
} // namespace idl
} // namespace maps
} // namespace yandex
