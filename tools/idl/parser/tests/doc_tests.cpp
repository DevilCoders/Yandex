#include "test_helpers.h"

#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
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
namespace doc_tests {

class Iterator : public nodes::StrictVisitor {
public:
    void onVisited(const nodes::Enum& e) override
    {
        BOOST_CHECK_EQUAL(e.name.original(), "E");
        BOOST_REQUIRE(e.doc);
        BOOST_CHECK_EQUAL(e.doc->description.format, "/*c2 ###*/");
    }
    void onVisited(const nodes::Interface& i) override
    {
        BOOST_CHECK_EQUAL(i.name.original(), "I");
        BOOST_REQUIRE(i.doc);
        BOOST_CHECK_EQUAL(i.doc->description.format,
            "/**\r\n * Some description...\n\r */");
    }
    void onVisited(const nodes::Listener& l) override
    {
        BOOST_CHECK_EQUAL(l.name.original(), "L");
        BOOST_REQUIRE(l.doc);
        BOOST_CHECK_EQUAL(l.doc->description.format,
            "//Listener used by %1%-es\n//second line of comments\n");
    }
    void onVisited(const nodes::Struct& s) override
    {
        BOOST_CHECK_EQUAL(s.name.original(), "S");
        BOOST_REQUIRE(s.doc);
        BOOST_CHECK_EQUAL(s.doc->description.format, "//c1 ...\n");
    }
    void onVisited(const nodes::Variant& v) override
    {
        BOOST_CHECK_EQUAL(v.name.original(), "V");
        BOOST_REQUIRE(v.doc);
        BOOST_CHECK_EQUAL(v.doc->description.format, "/*c3 / //* */");
    }
};

BOOST_AUTO_TEST_CASE(comments)
{
    BOOST_CHECK_THROW(parseIdl("", "/*/struct S{}"), utils::GroupedError);
    BOOST_CHECK_THROW(parseIdl("", "//asdf"), utils::GroupedError);
    parseIdl("",
        "\n"
        "///\n"
        "///some comments\n"
        "///\n"
        "interface I{}");

    const std::string VALID_TEXT =
        "//c1 ...\n"
        "struct S{}\n"
        "/*c2 ###*/enum E{F}/*c3 / //* */variant V{int i;}/**\r\n"
        " * Some description...\n\r"
        " */\n"
        "interface I{}\n"
        "//Listener used by {@link X.Y}-es\n"
        "//second line of comments\n"
        "listener L{void onV();}";
    const auto root = parseIdl("", VALID_TEXT);
    BOOST_CHECK_EQUAL(root.nodes.count(), 5);

    root.nodes.traverse(Iterator());
}

BOOST_AUTO_TEST_CASE(doc_parsing)
{
    const auto root = parseIdl("",
        "/**\n"
        " * Some comment   \n"
        "that spans multiple lines\n"
        " *\n"
        " * Comment on the new\n"
        " * * *paragraph.           \n"
        " *   \n"
        " *\n"
        " * Comment with links {@link some.ns.Some.Class} and {@link some.other.Class}\n"
        " /  * / ****link on new line\n"
        " */\n"
        "struct S { }");
    BOOST_CHECK_EQUAL(root.nodes.count(), 1);

    root.nodes.lambdaTraverse(
        [](const nodes::Struct& s)
        {
            BOOST_CHECK_EQUAL(s.name.original(), "S");
            BOOST_REQUIRE(s.doc);
            BOOST_CHECK_EQUAL(generateIdlDoc(s.doc->description),
                "/**\n"
                " * Some comment   \n"
                "that spans multiple lines\n"
                " *\n"
                " * Comment on the new\n"
                " * * *paragraph.           \n"
                " *   \n"
                " *\n"
                " * Comment with links some.ns.Some.Class and some.other.Class\n"
                " /  * / ****link on new line\n"
                " */");
            BOOST_CHECK(s.doc->parameters.empty());
            BOOST_CHECK(s.doc->result.format.empty());
            BOOST_CHECK(s.doc->result.links.empty());
        });
}

BOOST_AUTO_TEST_CASE(function_doc_parsing)
{
    const auto root = parseIdl("",
        "interface I {\n"
        "    /**\n"
        "     * Some function description\n"
        "    that spans multiple, very many\n"
        "     * lines and has some {@link link}s.\n"
        "     *\n"
        "     * New paragraph. Some more\n"
        "     * {@link links}s. Finished.\n"
        "     * @param someParam someDescription with {@link link} and\n"
        "     * {@link another.link}\n"
        "     * @param anotherParam\n"
        "     * @param p\n"
        "     * @return returns some value\n"
        "     * @param thirdParam - unused\n"
        "     */\n"
        "    void func();\n"
        "}");
    BOOST_CHECK_EQUAL(root.nodes.count(), 1);

    root.nodes.lambdaTraverse(
        [](const nodes::Interface& i)
        {
            i.nodes.lambdaTraverse(
                [](const nodes::Function f)
                {
                    BOOST_CHECK(f.name.original() == Scope{"func"});
                    BOOST_REQUIRE(f.doc);

                    const nodes::Doc& doc = *f.doc;
                    BOOST_CHECK_EQUAL(generateIdlDoc(doc.description),
                        "/**\n"
                        "     * Some function description\n"
                        "    that spans multiple, very many\n"
                        "     * lines and has some links.\n"
                        "     *\n"
                        "     * New paragraph. Some more\n"
                        "     * linkss. Finished.\n"
                        "     * ");
                    BOOST_CHECK_EQUAL(doc.parameters.size(), 4);
                    BOOST_CHECK_EQUAL(doc.parameters[0].first, "someParam");
                    BOOST_CHECK_EQUAL(doc.parameters[1].first, "anotherParam");
                    BOOST_CHECK_EQUAL(doc.parameters[2].first, "p");
                    BOOST_CHECK_EQUAL(doc.parameters[3].first, "thirdParam");
                    BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[0].second),
                        " someDescription with link and\n     * another.link\n     * ");
                    BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[1].second),
                        "\n     * ");
                    BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[2].second),
                        "\n     * ");
                    BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[3].second),
                        " - unused\n     */");
                    BOOST_CHECK_EQUAL(generateIdlDoc(doc.result),
                        " returns some value\n     * ");
                    BOOST_CHECK_EQUAL(doc.status,
                                      nodes::Doc::Status::Default);
                });
        });
}

BOOST_AUTO_TEST_CASE(internal_status_doc_parsing)
{
    const auto root = parseIdl("",
                               "interface I {\n"
                               "    /**\n"
                               "     * @internal\n"
                               "     * Some function description\n"
                               "    that spans multiple, very many\n"
                               "     * lines and has some {@link link}s.\n"
                               "     *\n"
                               "     * New paragraph. Some more\n"
                               "     * {@link links}s. Finished.\n"
                               "     * @param someParam someDescription with {@link link} and\n"
                               "     * {@link another.link}\n"
                               "     * @param anotherParam\n"
                               "     * @param p\n"
                               "     * @return returns some value\n"
                               "     * @param thirdParam - unused\n"
                               "     */\n"
                               "    void func();\n"
                               "}");
    BOOST_CHECK_EQUAL(root.nodes.count(), 1);

    root.nodes.lambdaTraverse(
            [](const nodes::Interface& i)
            {
                i.nodes.lambdaTraverse(
                        [](const nodes::Function f)
                        {
                            BOOST_CHECK(f.name.original() == Scope{"func"});
                            BOOST_REQUIRE(f.doc);

                            const nodes::Doc& doc = *f.doc;
                            BOOST_CHECK_EQUAL(generateIdlDoc(doc.description),
                                              "/**\n"
                                              "     * \n"
                                              "     * Some function description\n"
                                              "    that spans multiple, very many\n"
                                              "     * lines and has some links.\n"
                                              "     *\n"
                                              "     * New paragraph. Some more\n"
                                              "     * linkss. Finished.\n"
                                              "     * ");
                            BOOST_CHECK_EQUAL(doc.parameters.size(), 4);
                            BOOST_CHECK_EQUAL(doc.parameters[0].first, "someParam");
                            BOOST_CHECK_EQUAL(doc.parameters[1].first, "anotherParam");
                            BOOST_CHECK_EQUAL(doc.parameters[2].first, "p");
                            BOOST_CHECK_EQUAL(doc.parameters[3].first, "thirdParam");
                            BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[0].second),
                                              " someDescription with link and\n     * another.link\n     * ");
                            BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[1].second),
                                              "\n     * ");
                            BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[2].second),
                                              "\n     * ");
                            BOOST_CHECK_EQUAL(generateIdlDoc(doc.parameters[3].second),
                                              " - unused\n     */");
                            BOOST_CHECK_EQUAL(generateIdlDoc(doc.result),
                                              " returns some value\n     * ");
                            BOOST_CHECK_EQUAL(doc.status,
                                              nodes::Doc::Status::Internal);
                        });
            });
}


BOOST_AUTO_TEST_CASE(commercial_status_doc_parsing)
{
    const auto root = parseIdl("",
                               "interface I {\n"
                               "    /**\n"
                               "     * Some function description\n"
                               "    that spans multiple, very many\n"
                               "     *\n"
                               "     * New paragraph. Some more\n"
                               "     * @commercial\n"
                               "     */\n"
                               "    void func();\n"
                               "}");
    BOOST_CHECK_EQUAL(root.nodes.count(), 1);

    root.nodes.lambdaTraverse(
            [](const nodes::Interface& i)
            {
                i.nodes.lambdaTraverse(
                        [](const nodes::Function f)
                        {
                            BOOST_CHECK(f.name.original() == Scope{"func"});
                            BOOST_REQUIRE(f.doc);

                            const nodes::Doc& doc = *f.doc;
                            BOOST_CHECK_EQUAL(generateIdlDoc(doc.description),
                                              "/**\n"
                                              "     * Some function description\n"
                                              "    that spans multiple, very many\n"
                                              "     *\n"
                                              "     * New paragraph. Some more\n"
                                              "     * \n"
                                              "     */");
                            BOOST_CHECK_EQUAL(doc.status,
                                              nodes::Doc::Status::Commercial);
                        });
            });
}

} // namespace doc_tests
} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex
