#include "test_helpers.h"

#include <yandex/maps/idl/generator/java/generation.h>
#include <yandex/maps/idl/generator/jni_cpp/generation.h>
#include <yandex/maps/idl/generator/objc/generation.h>
#include <yandex/maps/idl/generator/cpp/generation.h>
#include <yandex/maps/idl/generator/obj_cpp/generation.h>
#include <yandex/maps/idl/idl.h>

#include <library/cpp/testing/common/env.h>

#include <boost/test/unit_test.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {

BOOST_AUTO_TEST_CASE(internal_tag_test)
{
    auto env = simpleEnv();
    auto idl = env.idl("docs/internal_tag_test.idl");

    auto generators = {
        generator::cpp::generate,
        generator::java::generate,
        generator::jni_cpp::generate,
        generator::objc::generate,
        generator::obj_cpp::generate
    };

    for (auto generator : generators) {
        for (auto file: generator(idl)) {
            try {
                std::string contents =
                    utils::Path(
                        std::string(ArcadiaSourceRoot().c_str())
                        + "/tools/idl/generator/tests/generated_files"
                        + file.suffixPath.fileName()
                    ).read();
                BOOST_CHECK_EQUAL(contents, file.text);
            } catch (utils::UsageError& error) {
                BOOST_CHECK_MESSAGE(false, error.what());
            }
        }
    }
}

} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
