#include "test_helpers.h"

#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
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
namespace interface_tests {

class InterfaceNodesIterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Function& f) override
    {
        if (f.name.original() == Scope("o")) {
            BOOST_CHECK(f.isConst);
            BOOST_CHECK(!f.result.typeRef.isOptional);
            BOOST_CHECK(f.parameters[1].typeRef.isOptional);
            BOOST_CHECK(f.parameters[1].typeRef.id == nodes::TypeId::Int);
            BOOST_CHECK(!f.parameters[3].typeRef.isOptional);
            BOOST_REQUIRE(f.doc);
            BOOST_CHECK_EQUAL(f.doc->description.format, "// comment2\n");
            BOOST_CHECK(f.result.typeRef.id == nodes::TypeId::Any);
            BOOST_CHECK(f.result.typeRef.isConst);
            BOOST_CHECK_EQUAL(f.name.original(), Scope("o"));
            BOOST_CHECK_EQUAL(f.parameters.size(), 5);
            BOOST_CHECK(f.parameters[0].typeRef.id == nodes::TypeId::Void);
            BOOST_CHECK_EQUAL(f.parameters[0].name, "d");
            BOOST_CHECK_EQUAL(f.parameters[1].name, "a");
            BOOST_CHECK(!f.parameters[2].typeRef.isConst);
            BOOST_CHECK_EQUAL(*f.parameters[2].typeRef.name, Scope("I2"));
            BOOST_REQUIRE(f.parameters[2].defaultValue);
            BOOST_CHECK_EQUAL(*f.parameters[2].defaultValue, "I2(k)");
            BOOST_CHECK(!f.parameters[3].typeRef.isConst);
            BOOST_CHECK(f.parameters[4].typeRef.isConst);
            BOOST_CHECK_EQUAL(f.parameters[4].name, "c");
            BOOST_CHECK_EQUAL(*f.parameters[4].defaultValue, "3");
        } else if (f.name.original() == Scope("method")) {
            BOOST_CHECK(f.result.typeRef.isOptional);
            BOOST_CHECK(f.result.typeRef.id == nodes::TypeId::Custom);
            BOOST_CHECK_EQUAL(*f.result.typeRef.name, Scope("I1"));
        } else {
            BOOST_FAIL("wrong method declaration in interface!");
        }
    }

    void onVisited(const nodes::Listener& l) override
    {
        BOOST_CHECK_EQUAL(l.name.original(), "L");
    }

    void onVisited(const nodes::Property& p) override
    {
        auto name = p.name;
        if (name == "boolProperty") {
            BOOST_CHECK(p.isReadonly);
            BOOST_CHECK_EQUAL(
                p.doc->description.format,
                "/**     * property comment     */");
            BOOST_CHECK(p.typeRef.id == nodes::TypeId::Bool);
        } else if (name == "intProperty") {
            BOOST_CHECK(p.isReadonly);
        } else if (name == "doubleProperty") {
            BOOST_CHECK(!p.isReadonly);
        } else {
            BOOST_ERROR("Unknown interface property '" + name + "'");
        }
    }

    void onVisited(const nodes::Struct& s) override
    {
        BOOST_CHECK_EQUAL(s.name.original(), "S");
    }
};

class IdlNodesIterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Interface& i) override
    {
        auto name = i.name.original();
        if (name == "I1") {
            BOOST_CHECK(!i.doc);
            BOOST_CHECK(!i.isVirtual);
            BOOST_CHECK(!i.isViewDelegate);
            BOOST_CHECK(!i.isStatic);
            BOOST_CHECK(i.ownership == nodes::Interface::Ownership::Weak);
            BOOST_CHECK(!i.base);
            BOOST_CHECK_EQUAL(i.nodes.count(), 1);
        } else if (name == "I2") {
            BOOST_REQUIRE(i.doc);
            BOOST_CHECK_EQUAL(i.doc->description.format, "/** * comment1 */");
            BOOST_CHECK(!i.isVirtual);
            BOOST_CHECK(i.isViewDelegate);
            BOOST_CHECK(!i.isStatic);
            BOOST_CHECK(i.ownership == nodes::Interface::Ownership::Weak);
            BOOST_CHECK(i.base);
            BOOST_CHECK_EQUAL(*i.base, Scope({"some", "other", "I2"}));
            BOOST_CHECK_EQUAL(i.nodes.count(), 7);

            i.nodes.traverse(InterfaceNodesIterator());
        } else if (name == "I3") {
            BOOST_CHECK(i.isVirtual);
            BOOST_CHECK(!i.isViewDelegate);
            BOOST_CHECK(!i.isStatic);
            BOOST_CHECK(i.ownership == nodes::Interface::Ownership::Strong);
        } else if (name == "I4") {
            BOOST_CHECK(i.ownership == nodes::Interface::Ownership::Shared);
        } else if (name == "SomeStatics") {
            BOOST_CHECK(!i.isVirtual);
            BOOST_CHECK(!i.isViewDelegate);
            BOOST_CHECK(i.isStatic);
            BOOST_CHECK_EQUAL(i.nodes.count<nodes::Function>(), 2);
            BOOST_CHECK_EQUAL(i.nodes.count<nodes::Property>(), 3);
        } else {
            BOOST_ERROR("Unknown interface '" + name + "'");
        }
    }

    void onVisited(const nodes::Listener& l) override
    {
        auto name = l.name.original();
        if (name == "PI") {
            BOOST_REQUIRE_EQUAL(l.functions.size(), 2);
            BOOST_CHECK_EQUAL(
                l.functions[0].name.original(), Scope("calculateUint"));
            BOOST_CHECK(l.functions[0].result.typeRef.id == nodes::TypeId::Uint);
            BOOST_CHECK_EQUAL(
                l.functions[1].name.original(), Scope("calculateFloat"));
            BOOST_CHECK(l.functions[1].result.typeRef.id == nodes::TypeId::Float);
            BOOST_CHECK_EQUAL(l.functions[1].parameters.size(), 2);
            BOOST_CHECK_EQUAL(l.functions[1].parameters[0].name, "i");
            BOOST_CHECK(l.functions[1].parameters[0].typeRef.id == nodes::TypeId::Int);
            BOOST_CHECK_EQUAL(l.functions[1].parameters[1].name, "f");
            BOOST_CHECK(
                l.functions[1].parameters[1].typeRef.id == nodes::TypeId::Float);
        } else if (name == "SPI") {
            BOOST_REQUIRE_EQUAL(l.functions.size(), 1);
            BOOST_CHECK_EQUAL(l.functions[0].name.original(), Scope("clone"));
            BOOST_CHECK(l.functions[0].result.typeRef.id == nodes::TypeId::Custom);
            BOOST_CHECK_EQUAL(*l.functions[0].result.typeRef.name, Scope("SPI"));
        } else {
            BOOST_ERROR("Unknown listener '" + name + "'");
        }
    }
};

BOOST_AUTO_TEST_CASE(interfaces)
{
    BOOST_CHECK_THROW(parseIdl("", "Interface I{}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "interface I()"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "interface I[]"), utils::GroupedError);
    parseIdl("", "interface I{}");
    BOOST_CHECK_THROW(parseIdl("",
        "interface I based on \"path\":Message {}"), utils::GroupedError);
    BOOST_CHECK_THROW(
        parseIdl("", "interface I{void do() const}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "interface I{property any o}"),
        utils::GroupedError);

    const std::string VALID_TEXT =
        "weak_ref interface I1 {"
        "    void doSomething(A a, int i, B b);"
        "}"
        ""
        "/**"
        " * comment1"
        " */"
        "view_delegate interface I2 : some.other.I2 {"
        "    // comment2\n"
        "    const any o("
        "        void d,"
        "        optional int a,"
        "        I2 i = I2(k),"
        "        int o,"
        "        const int c = 3) const;"
        "    struct S{}"
        "    listener L{void onEvent();}"
        "    optional I1 method() const;"
        ""
        "    /**"
        "     * property comment"
        "     */"
        "    bool boolProperty readonly;"
        "    int intProperty readonly;"
        "    double doubleProperty;"
        "}"
        ""
        "virtual interface I3 : some.other.I {"
        "    void method(bool b);"
        "}"
        ""
        "shared_ref interface I4 {"
        "}"
        ""
        "platform interface PI {"
        "    uint calculateUint();"
        "    float calculateFloat(int i, float f);"
        "}"
        ""
        "strong_ref platform interface SPI {"
        "    SPI clone();"
        "}"
        ""
        "static interface SomeStatics {"
        "    int a;"
        "    float b;"
        "    void doSomething(string s, char c);"
        "    double d;"
        "    SomeInterface createI();"
        "}";
    const auto root = parseIdl("", VALID_TEXT);
    BOOST_CHECK_EQUAL(root.nodes.count(), 7);

    root.nodes.traverse(IdlNodesIterator());
}

} // namespace interface_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
