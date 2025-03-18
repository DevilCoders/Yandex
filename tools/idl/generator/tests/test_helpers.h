#pragma once

#include <yandex/maps/idl/env.h>
#include <yandex/maps/idl/generator/output_file.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/utils/paths.h>

#include <ctemplate/template.h>

#include <boost/test/unit_test.hpp>

#include <functional>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {

Environment simpleEnv();
Environment protoEnv(const std::string& idlSearchPath);

/**
 * Produces empty dictionary, fills it with given lambda and then expands it
 * with given .tpl file and returns resulting string.
 */
std::string generateString(
    const std::string& tplDir,
    const std::string& tplName,
    std::function<void(ctemplate::TemplateDictionary* dict)> dictBuilder);

/**
 * Generates files based on a given .idl file.
 */
using IdlGenerator = std::function<std::vector<OutputFile>(const Idl*)>;

/**
 * Generates files based on all .idl files in given root directory.
 *
 * @return generated file paths
 */
std::vector<std::string> generateAllFiles(
    Environment* env,
    IdlGenerator idlGenerator);

/**
 * Used by checkTargets(...) method below.
 */
struct ExpectedTargetsGroup {
    std::string prefix;
    std::vector<std::string> suffixes;
};

/**
 * Checks whether generated targets are equal; gives helpful messages
 * otherwise.
 */
void checkTargets(
    const std::vector<std::string>& generatedTargetsVector,
    const std::vector<ExpectedTargetsGroup>& expectedTargetGroups);

} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
