#include "test_helpers.h"

#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/errors.h>

#include <memory>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace listener_tests {

class Iterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Interface& i) override
    {
        if (i.name.original() == "NL") {
            BOOST_CHECK(!i.isVirtual);
            BOOST_CHECK(i.ownership == nodes::Interface::Ownership::Strong);
            BOOST_CHECK(!i.isViewDelegate);
            BOOST_CHECK(!i.base);
            BOOST_CHECK_EQUAL(i.nodes.count(), 1);
        } else {
            BOOST_ERROR("Unknown interface '" + i.name.original() + "'");
        }
    }

    void onVisited(const nodes::Listener& l) override
    {
        if (l.name.original() == "L1") {
            BOOST_CHECK(l.isLambda);
            BOOST_CHECK(!l.doc);
            BOOST_CHECK(!l.base);
            BOOST_REQUIRE_EQUAL(l.functions.size(), 1);
        } else if (l.name.original() == "L2") {
            BOOST_CHECK(!l.isLambda);
            BOOST_REQUIRE(l.doc);
            BOOST_CHECK_EQUAL(l.doc->description.format, "/** * comment1 */");
            BOOST_REQUIRE(l.base);
            BOOST_CHECK_EQUAL(*l.base, Scope("L2"));
            BOOST_REQUIRE_EQUAL(l.functions.size(), 7);
            BOOST_CHECK_EQUAL(l.functions[0].doc->description.format,
                "// comment2\n");
            BOOST_CHECK(l.functions[0].result.typeRef.isConst);
            BOOST_CHECK_EQUAL(l.functions[0].name.original(), Scope("o"));
            BOOST_REQUIRE_EQUAL(l.functions[0].parameters.size(), 5);
            BOOST_CHECK(l.functions[1].result.typeRef.id == nodes::TypeId::Void);
            BOOST_CHECK(
                l.functions[1].threadRestriction == nodes::Function::ThreadRestriction::Ui);
            BOOST_REQUIRE_EQUAL(l.functions[1].parameters.size(), 2);
            BOOST_CHECK_EQUAL(*l.functions[1].parameters[0].typeRef.name,
                Scope("SomeInterface"));
            BOOST_CHECK(l.functions[1].parameters[1].typeRef.isOptional);
            BOOST_CHECK_EQUAL(l.functions[2].name.original(),
                Scope("onImageReceived"));
            BOOST_CHECK(
                l.functions[2].threadRestriction == nodes::Function::ThreadRestriction::Bg);
            BOOST_REQUIRE_EQUAL(l.functions[2].parameters.size(), 2);
            BOOST_CHECK(l.functions[2].parameters[0].typeRef.id ==
                nodes::TypeId::Bitmap);
            BOOST_CHECK(l.functions[2].parameters[0].typeRef.isConst);
            BOOST_CHECK_EQUAL(l.functions[3].name["objc"], Scope("iosName"));
            BOOST_CHECK_EQUAL(l.functions[3].name["cs"], Scope("csName"));
            BOOST_CHECK(
                l.functions[3].threadRestriction == nodes::Function::ThreadRestriction::None);
        } else {
            BOOST_ERROR("Unknown listener '" + l.name.original() + "'");
        }
    }
};

BOOST_AUTO_TEST_CASE(listeners)
{
    BOOST_CHECK_THROW(parseIdl("", "listener L{};"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "listener L()"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "listener L{}"), utils::GroupedError);
    parseIdl("", "listener L{void do();}");
    BOOST_CHECK_THROW(parseIdl("",
        "listener L based on \"path\":Message {}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("",
        "listener L{object o;}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("",
        "listener L{void do() const readonly;}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("",
        "listener L:virtual L2{void do();}"), utils::GroupedError);

    const std::string VALID_TEXT =
        "lambda listener L1 {"
        "    void doSomething(A a, int i, B b);"
        "}"
        "/**"
        " * comment1"
        " */"
        "listener L2 : L2 {"
        "    // comment2\n"
        "    const object o(void d, int a, I2 i, int o, int c);"
        "    void onTap(SomeInterface i, optional bool someBoolean);"
        "    bool onImageReceived(const bitmap bitmap, const image_provider provider) bg_thread;"
        "    void onParameterlessCallback<objc:iosName, cs:csName, java:javaName>() any_thread;"
        "    bool onAnimatedImageReceived(const animated_image_provider provider);"
        "    bool onModelReceived(const model_provider provider);"
        "    bool onAnimatedModelReceived(const animated_model_provider provider);"
        "}"
        "native listener NL {"
        "    void onSomeEvent(int eventCode);"
        "}";
    const auto root = parseIdl("", VALID_TEXT);
    BOOST_CHECK_EQUAL(root.nodes.count(), 3);

    root.nodes.traverse(Iterator());
}

} // namespace listener_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
