#include "objc/common.h"

#include "common/common.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils/common.h>

#include <boost/regex.hpp>

#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

namespace {

std::string filePathFromTypeName(
    const std::string& frameworkName,
    const std::string& typeName,
    bool isHeader)
{
    return frameworkName + '/' + typeName + (isHeader ? ".h" : ".m");
}

} // namespace

utils::Path filePath(const Idl* idl, bool isHeader)
{
    auto prefix = idl->objcTypePrefix.asPrefix("");
    std::string suffix = idl->relativePath.stem().camelCased(true);

    return filePathFromTypeName(
        idl->objcFramework, prefix + suffix, isHeader);
}

utils::Path filePath(
    const Idl* idl,
    const nodes::Name& topLevelTypeName,
    bool isHeader)
{
    auto prefix = idl->objcTypePrefix.asPrefix("");

    return filePathFromTypeName(
        idl->objcFramework, prefix + topLevelTypeName[OBJC], isHeader);
}

utils::Path filePath(const TypeInfo& typeInfo, bool isHeader)
{
    auto prefix = typeInfo.idl->objcTypePrefix.asPrefix("");

    std::string suffix;
    if (typeInfo.scope.original().isEmpty()) {
        auto typePathStem = typeInfo.idl->relativePath.stem();
        suffix = utils::toCamelCase(typePathStem, true);
    } else {
        suffix = typeInfo.scope[OBJC].first();
    }

    return filePathFromTypeName(
        typeInfo.idl->objcFramework, prefix + suffix, isHeader);
}

std::string alignParameters(const std::string& text)
{
    static const boost::regex TEXT_REGEX(
        "([^:\\n]+:[^:\\s][^;\\n]+\\n)+" /* lines with ':' but without ';' */
        "([^:\\n]+:[^:\\s][^\\n]+\\n)?" /* last line - can have ';'-s */
    );
    static const boost::regex LINE_REGEX("(\\s*)(([^:]+):[^\\n]+\\n)");

    return boost::regex_replace(text, TEXT_REGEX,
        [](boost::smatch match)
        {
            std::string text = std::string(match[0]);
            std::string alignedText;

            int alignmentPosition = 0;
            std::for_each(
                boost::sregex_iterator(text.begin(), text.end(), LINE_REGEX),
                boost::sregex_iterator(),
                [&alignedText, &alignmentPosition](boost::smatch m)
                {
                    if (alignmentPosition == 0) { // first line
                        alignmentPosition = m[1].length() + m[3].length();
                        alignedText += std::string(m[0]);
                    } else {
                        auto diff = int(alignmentPosition) - m[3].length();
                        if (diff > 0) {
                            alignedText += std::string(diff, ' ');
                        } // else - too long prefix for line realignment

                        alignedText += std::string(m[2]);
                    }
                }
            );
            return alignedText;
        }
    );
}

std::string runtimeFrameworkPrefix(const Environment* env)
{
    return env->runtimeFramework()->objcFrameworkPrefix.asString("");
}

void setRuntimeFrameworkPrefix(
    const Environment* env,
    ctemplate::TemplateDictionary* dict)
{
    dict->SetValue(
        "OBJC_RUNTIME_FRAMEWORK_PREFIX",
        runtimeFrameworkPrefix(env));
}

bool supportsOptionalAnnotation(const FullTypeRef& typeRef)
{
    return typeRef.isObjCNullable() || typeRef.isOptional();
}

bool hasNullableAnnotation(const FullTypeRef& typeRef)
{
    return typeRef.isOptional();
}


} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
