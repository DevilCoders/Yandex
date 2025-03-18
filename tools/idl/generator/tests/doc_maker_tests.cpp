#include "tests/test_helpers.h"

#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "cpp/enum_field_maker.h"
#include "cpp/function_maker.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"
#include "java/doc_maker.h"
#include "java/enum_field_maker.h"
#include "java/import_maker.h"
#include "java/type_name_maker.h"
#include "objc/doc_maker.h"
#include "objc/enum_field_maker.h"
#include "objc/function_maker.h"
#include "objc/import_maker.h"
#include "objc/pod_decider.h"
#include "objc/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/doc.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/targets.h>

#include <ctemplate/template.h>

#include <boost/test/unit_test.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {

class DocMakerTestVisitor : public nodes::Visitor {
public:
    DocMakerTestVisitor(const Idl* idl)
        : idl_(idl)
    {
    }

    using nodes::Visitor::onVisited;

    void onVisited(const nodes::Interface& i) override
    {
        if (i.name.original() == "InterfaceWithDocs") {
            i.nodes.traverse(this);
        }
    }

    void onVisited(const nodes::Function& m) override
    {
        BOOST_CHECK_EQUAL(m.name.original(), Scope("methodWithDocs"));
        BOOST_CHECK(!m.isConst);
        BOOST_REQUIRE(m.doc);

        Scope scope("InterfaceWithDocs");

        auto doxygen = generateString("test", "doc_test.tpl",
            [this, &m, &scope](ctemplate::TemplateDictionary* dict)
            {
                common::PodDecider podDecider;
                cpp::TypeNameMaker typeNameMaker;
                cpp::ImportMaker importMaker(idl_, &typeNameMaker);
                cpp::EnumFieldMaker enumFieldMaker(&typeNameMaker);
                cpp::FunctionMaker functionMaker(
                    &podDecider, &typeNameMaker, &importMaker, false);
                common::DocMaker docMaker("cpp_docs", &typeNameMaker,
                    &importMaker, &enumFieldMaker, &functionMaker);
                docMaker.make(dict, "DOCS", { idl_, scope }, m.doc);
            }
        );
        BOOST_CHECK_EQUAL(doxygen,
            R"(/**
 * Link to ::test::docs::Struct, to ::test::docs::Variant::f, and some
 * unsupported tag {@some.unsupported.tag}.
 *
 * More links after separator: ::test::docs::Struct,
 * ::test::docs::Interface::method(int, float, const
 * ::test::docs::Struct&, ::test::docs::Variant&), and link to self:
 * methodWithDocs(::test::docs::Interface*, const ::test::docs::Struct&,
 * const ::test::docs::OnResponse&, const ::test::docs::OnError&).
 *
 * @param i - ::test::docs::Interface, does something important
 * @param s - some struct
 *
 * @return true if successful, false - otherwise
 */
)");

        auto javadoc = generateString("test", "doc_test.tpl",
            [this, &m, &scope](ctemplate::TemplateDictionary* dict)
            {
                common::PodDecider podDecider;
                java::TypeNameMaker typeNameMaker;
                java::ImportMaker importMaker(idl_, &typeNameMaker, false);
                java::EnumFieldMaker enumFieldMaker(&typeNameMaker);
                common::FunctionMaker functionMaker(&podDecider,
                    &typeNameMaker, &importMaker, JAVA, false);
                java::DocMaker docMaker("java_docs", &typeNameMaker,
                    &importMaker, &enumFieldMaker, &functionMaker);
                docMaker.make(dict, "DOCS", { idl_, scope }, m.doc);
            }
        );
        BOOST_CHECK_EQUAL(javadoc,
            R"(/**
 * Link to {@link Struct}, to {@link Variant#f}, and some unsupported
 * tag {@some.unsupported.tag}.
 *
 * More links after separator: {@link Struct}, {@link
 * JavaInterface#method(int, float, Struct, Variant)}, and link to self:
 * {@link #methodWithDocs(JavaInterface, Struct, LambdaListener)}.
 *
 * @param i - {@link JavaInterface}, does something important
 * @param s - some struct
 *
 * @return true if successful, false - otherwise
 */
)");

        auto objcDocs = generateString("test", "doc_test.tpl",
            [this, &m, &scope](ctemplate::TemplateDictionary* dict)
            {
                objc::PodDecider podDecider;
                objc::TypeNameMaker typeNameMaker;
                objc::ImportMaker importMaker(idl_, &typeNameMaker, true);
                objc::EnumFieldMaker enumFieldMaker(&typeNameMaker);
                objc::FunctionMaker functionMaker(&podDecider,
                    &typeNameMaker, &importMaker, OBJC, false);
                objc::DocMaker docMaker("objc_docs", &typeNameMaker,
                    &importMaker, &enumFieldMaker, &functionMaker);
                docMaker.make(dict, "DOCS", { idl_, scope }, m.doc);
            }
        );
        BOOST_CHECK_EQUAL(objcDocs,
            R"(/**
 * Link to YTDStruct, to YTDVariant::f, and some unsupported tag
 * {@some.unsupported.tag}.
 *
 * More links after separator: YTDStruct,
 * YTDObjcInterface::methodWithIntValue:floatValue:someStruct:andVariant:,
 * and link to self: methodWithDocsWithI:s:responseHandler:.
 *
 * @param i - YTDObjcInterface, does something important
 * @param s - some struct
 *
 * @return true if successful, false - otherwise
 */
)");
    }

private:
    const Idl* idl_;
};

BOOST_AUTO_TEST_CASE(doc_maker_test)
{
    auto env = simpleEnv();

    auto idl = env.idl("docs/test.idl");
    idl->root.nodes.traverse(DocMakerTestVisitor(idl));
}

} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
