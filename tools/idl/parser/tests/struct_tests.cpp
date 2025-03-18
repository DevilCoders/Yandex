#include "test_helpers.h"

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/utils/errors.h>

#include <memory>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {
namespace struct_tests {

class StructNodesIterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Enum& e) override
    {
        BOOST_CHECK_EQUAL(e.name.original(), "E");
        BOOST_CHECK_EQUAL(e.fields.size(), 1);
        BOOST_CHECK_EQUAL(e.fields[0].name, "Field");
    }
    void onVisited(const nodes::Struct& s) override
    {
        BOOST_CHECK_EQUAL(s.name.original(), "S");
        BOOST_CHECK_EQUAL(s.nodes.count(), 3);
    }
    void onVisited(const nodes::StructField& f) override
    {
        if (f.name == "s1") {
            BOOST_REQUIRE(f.typeRef.name);
            BOOST_CHECK_EQUAL(*f.typeRef.name, Scope("S1"));
            BOOST_REQUIRE(f.defaultValue);
            BOOST_CHECK_EQUAL(*f.defaultValue,
                "callSomeMethod(\"some \')param\" \'))))\')");
        } else if (f.name == "i") {
            BOOST_CHECK(!f.typeRef.isOptional);
            BOOST_REQUIRE(f.defaultValue);
            BOOST_CHECK_EQUAL(*f.defaultValue, "5");
            BOOST_CHECK(!f.protoField);
        } else if (f.name == "i64") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Int64);
        } else if (f.name == "i32") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Int);
        } else if (f.name == "d") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Double);
            BOOST_CHECK(f.defaultValue);
            BOOST_REQUIRE(f.protoField);
            BOOST_CHECK_EQUAL(*f.protoField, "k");
        } else if (f.name == "vs") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Vector);
            BOOST_CHECK_EQUAL(f.typeRef.parameters.size(), 1);
            BOOST_CHECK(!f.defaultValue);
        } else if (f.name == "ms") {
            BOOST_CHECK(f.typeRef.isOptional);
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Dictionary);
            BOOST_CHECK_EQUAL(f.typeRef.parameters.size(), 2);
            BOOST_CHECK_EQUAL(*f.typeRef.parameters[1].name, Scope("V"));
            BOOST_CHECK(!f.defaultValue);
            BOOST_CHECK_EQUAL(*f.protoField, "mms");
        } else if (f.name == "e") {
            BOOST_CHECK_EQUAL(*f.typeRef.name, Scope("E"));
        } else if (f.name == "anything") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Any);
            BOOST_CHECK(!f.typeRef.name);
        } else if (f.name == "s") {
            BOOST_CHECK(f.typeRef.isOptional);
        } else if (f.name == "v") {
            BOOST_CHECK_EQUAL(*f.typeRef.name, Scope("V"));
        } else if (f.name == "ti") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::TimeInterval);
        } else if (f.name == "abs") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::AbsTimestamp);
        } else if (f.name == "rel") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::RelTimestamp);
        } else if (f.name == "bitmap") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Bitmap);
        } else if (f.name == "data") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Bytes);
        } else if (f.name == "color") {
            BOOST_CHECK(f.typeRef.id == nodes::TypeId::Color);
        } else {
            BOOST_FAIL("wrong field declaration in struct!");
        }
    }
    void onVisited(const nodes::Variant& v) override
    {
        BOOST_CHECK_EQUAL(v.name.original(), "V");
        BOOST_CHECK_EQUAL(v.fields.size(), 1);
        BOOST_CHECK_EQUAL(v.fields[0].name, "i");
    }
};

class IdlNodesIterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Struct& s) override
    {
        if (s.name.original() == "S1") {
            BOOST_CHECK(s.kind == nodes::StructKind::Options);
            BOOST_CHECK(s.protoMessage);
            BOOST_CHECK_EQUAL(s.nodes.count(), 0);
        } else if (s.name.original() == "S2") {
            BOOST_CHECK(s.kind == nodes::StructKind::Bridged);
            BOOST_CHECK(!s.protoMessage);
            BOOST_CHECK_EQUAL(s.nodes.count(), 20);
            BOOST_CHECK(s.kind == nodes::StructKind::Bridged);

            s.nodes.traverse(StructNodesIterator());
        } else if (s.name.original() == "S3") {
            BOOST_CHECK(s.kind == nodes::StructKind::Lite);
        } else {
            BOOST_FAIL("wrong struct name!");
        }
    }
};

BOOST_AUTO_TEST_CASE(structs)
{
    BOOST_CHECK_THROW(parseIdl("", "struct S"), utils::GroupedError);

    const std::string VALID_TEXT =
        "options struct S1 based on \"a/b.proto\":c.d {"
        "}"
        "struct S2 {"
        "    /**\n"
        "     * some comment\n"
        "     */\n"
        "    S1 s1 = callSomeMethod(\"some \')param\" \'))))\');"
        "    int i = 5;"
        "    int64 i64;"
        "    int i32;"
        "    optional double d based on k = SOME_VAL;"
        "    vector<V> vs;"
        "    optional dictionary<K,V> ms based on mms;"
        "    enum E {"
        "        Field"
        "    }"
        "    E e based on e;"
        "    struct S {"
        "        optional O o;"
        "        point p;"
        "        optional point optionalPoint;"
        "    }"
        "    any anything;"
        "    optional S s;"
        "    variant V {"
        "        int i;"
        "    }"
        "    V v;"
        "    time_interval ti;"
        "    abs_timestamp abs;"
        "    rel_timestamp rel;"
        "    bitmap bitmap;"
        "    bytes data;"
        "    color color;"
        "}"
        "lite struct S3 {"
        "   int x;"
        "}";
    const auto root = parseIdl("", VALID_TEXT);
    BOOST_CHECK_EQUAL(root.nodes.count(), 3);

    root.nodes.traverse(IdlNodesIterator());
}

} // namespace struct_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
