#include <yandex/maps/idl/generator/java/generation.h>

#include "common/common.h"
#include "common/enum_maker.h"
#include "common/generator.h"
#include "common/pod_decider.h"
#include "java/doc_maker.h"
#include "java/enum_field_maker.h"
#include "java/function_maker.h"
#include "java/import_maker.h"
#include "java/interface_maker.h"
#include "java/listener_maker.h"
#include "java/struct_maker.h"
#include "java/type_name_maker.h"
#include "java/variant_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/function.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils.h>

#include <boost/algorithm/string.hpp>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

namespace {

class TopLevelVisitor : public nodes::Visitor {
public:
    TopLevelVisitor(const Idl* idl)
        : idl_(idl)
    {
    }

    std::vector<OutputFile> files() const
    {
        return files_;
    }

private:
    void onVisited(const nodes::Enum& e) override
    {
        generateNodeFile(e);
    }
    void onVisited(const nodes::Interface& i) override
    {
        generateNodeFile(i, false);
        if (!i.isStatic) {
            generateNodeFile(i, true);
        }
    }
    void onVisited(const nodes::Listener& l) override
    {
        generateNodeFile(l);
    }
    void onVisited(const nodes::Struct& s) override
    {
        generateNodeFile(s);
    }
    void onVisited(const nodes::Variant& v) override
    {
        generateNodeFile(v);
    }

private:
    template <typename Node>
    void generateNodeFile(const Node& n, bool isBinding = false)
    {
        if (!needGenerateNode({idl_, {}}, n))
            return;

        tpl::DirGuard guard("java");

        ctemplate::TemplateDictionary rootDict("ROOT");

        if (isBinding) {
            rootDict.ShowSection("IS_BINDING");
        }

        common::PodDecider podDecider;
        TypeNameMaker typeNameMaker;
        ImportMaker importMaker(idl_, &typeNameMaker, isBinding);
        EnumFieldMaker enumFieldMaker(&typeNameMaker);
        FunctionMaker functionMaker(
            &podDecider, &typeNameMaker, &importMaker, JAVA, !isBinding);
        DocMaker docMaker("java_docs", &typeNameMaker, &importMaker,
            &enumFieldMaker, &functionMaker);
        DocMaker* docMakerPtr = isBinding ? nullptr : &docMaker;

        common::EnumMaker e(&typeNameMaker, &enumFieldMaker, docMakerPtr);
        InterfaceMaker i(&podDecider, &typeNameMaker, &importMaker,
            docMakerPtr, &functionMaker, isBinding);
        ListenerMaker l(
            &typeNameMaker, &importMaker, docMakerPtr, &functionMaker);
        StructMaker s(&podDecider, &typeNameMaker, &enumFieldMaker,
            &importMaker, docMakerPtr);
        VariantMaker v(
            &podDecider, &typeNameMaker, &importMaker, docMakerPtr);

        class InterfaceOnlyFilter : public common::Filter<false> {
        public:
            bool isUseful(
                const Scope& /* scope */,
                const nodes::Interface& /* s */) const override
            {
                return true;
            }
        } interfaceOnlyFilter;
        auto filter = isBinding ? &interfaceOnlyFilter : nullptr;
        common::Generator(&rootDict, { &e, &i, &l, &s, &v }, idl_, filter).onVisited(n);

        common::setNamespace(idl_->javaPackage, &rootDict);

        importMaker.fill(isBinding ? "" :
            idl_->javaPackage.asPrefix(".") + n.name[JAVA], &rootDict);

        auto filePath =
            idl_->javaPackage.asPath() + (isBinding ? "internal/" : "") +
            utils::Path(n.name[JAVA]).
                withSuffix(isBinding ? "Binding" : "").
                withExtension("java");

        files_.emplace_back(
            idl_->env->config.outAndroidImplRoot, filePath,
            tpl::expand("file.tpl", &rootDict),
            importMaker.getImportPaths());
    }

private:
    const Idl* idl_;

    std::vector<OutputFile> files_;
};

} // namespace

std::vector<OutputFile> generate(const Idl* idl)
{
    TopLevelVisitor visitor(idl);
    idl->root.nodes.traverse(&visitor);
    return visitor.files();
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
