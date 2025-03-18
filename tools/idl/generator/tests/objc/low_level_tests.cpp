#include "objc/common.h"

#include <boost/test/unit_test.hpp>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {

BOOST_AUTO_TEST_CASE(objc_parameter_alignment_test)
{
    std::string original =
        "+ (YMKSearchTimeRange *)timeRangeWithIsTwentyFourHours:(NSNumber *)isTwentyFourHours\n"
        "                        where:(NSNumber *)where\n"
        "                        from:(NSNumber *)from\n"
        "                        to:(NSNumber *)to;\n"
        "end\n"
        "\n"
        "    _text = [archive addString:_text\n"
        "                          optional:NO];\n"
        "\n"
        "short first line:suffix\n"
        "very long second ling:suffix2\n"
        "     third line:suffix3\n";
    BOOST_CHECK_EQUAL(objc::alignParameters(original),
        "+ (YMKSearchTimeRange *)timeRangeWithIsTwentyFourHours:(NSNumber *)isTwentyFourHours\n"
        "                                                 where:(NSNumber *)where\n"
        "                                                  from:(NSNumber *)from\n"
        "                                                    to:(NSNumber *)to;\n"
        "end\n"
        "\n"
        "    _text = [archive addString:_text\n"
        "                      optional:NO];\n"
        "\n"
        "short first line:suffix\n"
        "very long second ling:suffix2\n"
        "      third line:suffix3\n");
}

} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
