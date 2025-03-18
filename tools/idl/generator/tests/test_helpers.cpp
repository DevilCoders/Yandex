#include "tests/test_helpers.h"

#include "common/common.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <set>
#include <unordered_set>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {

namespace {

const char* const OUT_DIR = "build/test_output";

struct RemoveTestOutput {
    /**
     * Before tests begin, removes previous tests output.
     */
    RemoveTestOutput()
    {
        utils::Path(OUT_DIR).remove();
    }
};
BOOST_GLOBAL_FIXTURE(RemoveTestOutput);

} // namespace

Environment simpleEnv()
{
    return Environment({
        "", // inProtoRoot
        { "generator/tests/idl_frameworks" },
        { "generator/tests/idl_files" },
        "", // baseProtoPackage
        std::string(OUT_DIR) + "/base/include",
        std::string(OUT_DIR) + "/base/impl",
        std::string(OUT_DIR) + "/android/include",
        std::string(OUT_DIR) + "/android/impl",
        std::string(OUT_DIR) + "/ios/include",
        std::string(OUT_DIR) + "/ios/impl",
        false,
        false,
        false
    });
}

Environment protoEnv(const std::string& idlSearchPath)
{
    return Environment({
        { "generator/tests/protoconv/proto_root" },
        { "generator/tests/idl_frameworks" },
        { idlSearchPath },
        "yandex.maps.proto",
        std::string(OUT_DIR) + "/base/include",
        std::string(OUT_DIR) + "/base/impl",
        std::string(OUT_DIR) + "/android/include",
        std::string(OUT_DIR) + "/android/impl",
        std::string(OUT_DIR) + "/ios/include",
        std::string(OUT_DIR) + "/ios/impl",
        false,
        false,
        false
    });
}

std::string generateString(
    const std::string& tplDir,
    const std::string& tplName,
    std::function<void(ctemplate::TemplateDictionary* dict)> dictBuilder)
{
    tpl::DirGuard guard(tplDir);

    ctemplate::TemplateDictionary dict("TEST_DICTIONARY");
    dictBuilder(&dict);
    return tpl::expand(tplName, &dict);
}

std::vector<std::string> generateAllFiles(
    Environment* env,
    IdlGenerator idlGenerator)
{
    std::vector<std::string> filePathsVector;
    for (const auto& searchPath : env->config.inIdlSearchPaths) {
        for (const auto& idlPath : searchPath.childPaths(true, ".idl")) {
            const auto& files = idlGenerator(env->idl(idlPath));

            // Check duplicate targets
            std::unordered_set<std::string> filePathsSet;
            for (const auto& file : files) {
                if (!filePathsSet.insert(file.fullPath()).second) {
                    BOOST_TEST_MESSAGE("Duplicate file path: " + file.fullPath());
                }
            }

            // Write generated files
            for (const auto& file : files) {
                BOOST_REQUIRE(!file.text.empty());

                filePathsVector.push_back(file.fullPath().asString());
                file.fullPath().write(file.text);
            }
        }
    }
    return filePathsVector;
}

void checkTargets(
    const std::vector<std::string>& generatedTargetsVector,
    const std::vector<ExpectedTargetsGroup>& expectedTargetGroups)
{
    // Generated targets must not have duplicates
    std::set<std::string> duplicateGeneratedTargets;
    std::set<std::string> generatedTargets;
    for (const auto& target : generatedTargetsVector) {
        if (!generatedTargets.insert(target).second) {
            duplicateGeneratedTargets.insert(target);
        }
    }
    if (duplicateGeneratedTargets.size() > 0) {
        BOOST_ERROR("Following targets were generated more then once:\n\t" +
            boost::join(duplicateGeneratedTargets, "\n\t"));
    }

    // Correct targets must not have duplicates
    std::set<std::string> duplicateTargetSuffixes;
    std::set<std::string> correctTargets;
    for (const auto& targetsGroup : expectedTargetGroups) {
        for (const auto& suffix : targetsGroup.suffixes) {
            if(!correctTargets.insert(targetsGroup.prefix + suffix).second) {
                duplicateTargetSuffixes.insert(suffix);
            }
        }
    }
    if (duplicateTargetSuffixes.size() > 0) {
        BOOST_ERROR("Following correct target suffixes were provided more "
            "then once:\n\t" + boost::join(duplicateTargetSuffixes, "\n\t"));
    }

    // All correct targets must be generated
    std::vector<std::string> difference;
    std::set_difference(correctTargets.begin(), correctTargets.end(),
        generatedTargets.begin(), generatedTargets.end(),
        std::inserter(difference, difference.begin()));
    if (difference.size() > 0) {
        BOOST_ERROR("Following targets were not generated:\n\t" +
            boost::join(difference, "\n\t"));
    }

    // No unexpected targets must be generated
    difference.clear();
    std::set_difference(generatedTargets.begin(), generatedTargets.end(),
        correctTargets.begin(), correctTargets.end(),
        std::inserter(difference, difference.begin()));
    if (difference.size() > 0) {
        BOOST_ERROR("Following unexpected targets were generated:\n\t" +
            boost::join(difference, "\n\t"));
    }
}

} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
