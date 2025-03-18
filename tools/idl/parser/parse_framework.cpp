#include <yandex/maps/idl/parser/parse_framework.h>

#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/errors.h>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

namespace {

const boost::regex REGEX(
    "\\s*"
    "((BUNDLE_GUARD)|"
    "(CPP_NAMESPACE)|"
    "(CS_NAMESPACE)|"
    "(JAVA_PACKAGE)|"
    "(OBJC_FRAMEWORK)|(OBJC_FRAMEWORK_PREFIX))"
    "\\s*=\\s*(\\S+)");

inline void handleValueNotFound(
    const std::string& filePath,
    const std::string& valueName)
{
    throw utils::UsageError() <<
        utils::asConsoleBold("Couldn't find " + valueName +
            " in Framework file '" + filePath + "'");
}

} // namespace

Framework parseFramework(
    const std::string& filePath,
    const std::string& fileContents)
{
    boost::sregex_iterator iterator(
        fileContents.begin(), fileContents.end(), REGEX);
    boost::sregex_iterator end;

    std::string bundleGuard;
    Scope cppNamespace, csNamespace, javaPackage, objcFrameworkPrefix;
    std::string objcFramework;

    for (; iterator != end; ++iterator) {
        const auto& match = *iterator;
        if (match[2].matched) {
            bundleGuard = match[8];
        } else if (match[3].matched) {
            cppNamespace = Scope(match[8], '.');
        } else if (match[4].matched) {
            csNamespace = Scope(match[8], '.');
        } else if (match[5].matched) {
            javaPackage = Scope(match[8], '.');
        } else if (match[6].matched) {
            objcFramework = match[8];
        } else if (match[7].matched) {
            objcFrameworkPrefix = Scope(match[8], '.');
        }
    }

    if (bundleGuard.empty())
        bundleGuard = "NO_BUNDLE_GUARD_IN_DOT_FRAMEWORK_FILE";
    if (cppNamespace.isEmpty())
        handleValueNotFound(filePath, "C++ namespace");
    if (csNamespace.isEmpty())
        csNamespace = Scope(objcFramework);
    if (javaPackage.isEmpty())
        handleValueNotFound(filePath, "Java package");
    if (objcFramework.empty())
        handleValueNotFound(filePath, "Objective-C framework");
    if (objcFrameworkPrefix.isEmpty())
        handleValueNotFound(filePath, "Objective-C framework prefix");

    return {
        std::move(bundleGuard),

        std::move(cppNamespace),
        std::move(csNamespace),
        std::move(javaPackage),
        std::move(objcFramework),

        std::move(objcFrameworkPrefix)
    };
}

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
