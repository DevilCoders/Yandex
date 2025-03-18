#include <yandex/maps/idl/generator/cpp/generation.h>

#include "common/common.h"
#include "common/doc_maker.h"
#include "common/generator.h"
#include "common/pod_decider.h"
#include "cpp/bitfield_operators.h"
#include "cpp/common.h"
#include "cpp/enum_field_maker.h"
#include "cpp/enum_maker.h"
#include "cpp/function_maker.h"
#include "cpp/guid_registration_maker.h"
#include "cpp/import_maker.h"
#include "cpp/interface_maker.h"
#include "cpp/listener_maker.h"
#include "cpp/serialization_addition.h"
#include "cpp/struct_maker.h"
#include "cpp/traits_makers.h"
#include "cpp/type_name_maker.h"
#include "cpp/variant_maker.h"
#include "cpp/weak_ref_create_platforms.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

namespace {

class TargetsFinder : public nodes::Visitor {
public:
    TargetsFinder(const nodes::Root& root)
    {
        root.nodes.traverse(this);
    }

    bool headerWillBeGenerated() const { return headerWillBeGenerated_; }
    bool sourceWillBeGenerated() const { return sourceWillBeGenerated_; }

private:
    bool headerWillBeGenerated_{ false };
    bool sourceWillBeGenerated_{ false };

    void onVisited(const nodes::Enum& e) override
    {
        if (!e.customCodeLink.baseHeader) {
            headerWillBeGenerated_ = true;
        }
    }
    void onVisited(const nodes::Interface& /* i */) override
    {
        headerWillBeGenerated_ = true;
    }
    void onVisited(const nodes::Listener& /* l */) override
    {
        headerWillBeGenerated_ = true;
    }
    void onVisited(const nodes::Struct& s) override
    {
        if (!s.customCodeLink.baseHeader) {
            headerWillBeGenerated_ = sourceWillBeGenerated_ = true;
        }

        // No need to traverse inside - if this structure has custom header,
        // then inner "types" will also be custom-declared. The opposite is
        // also true - if inner struct is 'custom', outers will be too.
    }
    void onVisited(const nodes::Variant& v) override
    {
        if (!v.customCodeLink.baseHeader) {
            headerWillBeGenerated_ = true;
        }
    }
};

void generateTypeDeclarations(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict,
    bool isHeader)
{
    common::PodDecider podDecider;
    EnumFieldMaker enumFieldMaker(typeNameMaker);
    FunctionMaker functionMaker(
        &podDecider, typeNameMaker, importMaker, isHeader);
    common::DocMaker docMaker("cpp_docs", typeNameMaker,
        importMaker, &enumFieldMaker, &functionMaker);

    EnumMaker e(typeNameMaker, &enumFieldMaker, &docMaker, importMaker);
    InterfaceMaker i(&podDecider, typeNameMaker, importMaker,
        &docMaker, &functionMaker, isHeader);
    ListenerMaker l(
        typeNameMaker, importMaker, &docMaker, &functionMaker);
    StructMaker s(&podDecider, typeNameMaker, &enumFieldMaker,
        importMaker, &docMaker, isHeader);
    VariantMaker v(&podDecider, typeNameMaker, importMaker, &docMaker);

    class StructOnlyFilter : public common::Filter<false> {
    public:
        bool isUseful(
            const Scope& /* scope */,
            const nodes::Struct& /* s */) const override
        {
            return true;
        }
    } structOnlyFilter;
    auto filter = isHeader ? nullptr : &structOnlyFilter;

    common::Generator gen(rootDict, { &e, &i, &l, &s, &v }, idl, filter);
    idl->root.nodes.traverse(&gen);
}

void generateBitfieldOperators(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker,
    ctemplate::TemplateDictionary* rootDict)
{
    BitfieldOpsEnumMaker maker(typeNameMaker, rootDict);
    common::Makers makers{ &maker, nullptr, nullptr, nullptr, nullptr };

    idl->root.nodes.traverse(common::Generator(rootDict, makers, idl));
}

void generateWeakRefCreatePlatforms(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict)
{
    CreatePlatformsInterfaceMaker maker(typeNameMaker, importMaker, rootDict);
    common::Makers makers{ nullptr, &maker, nullptr, nullptr, nullptr };

    idl->root.nodes.traverse(common::Generator(rootDict, makers, idl));
}

void generateExternalSerialization(
    const Idl* idl,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict,
    bool isHeader)
{
    ExtSerialStructMaker maker(
        typeNameMaker, importMaker, rootDict, isHeader);
    common::Makers makers{ nullptr, nullptr, nullptr, &maker, nullptr };

    idl->root.nodes.traverse(common::Generator(rootDict, makers, idl));
}

void generateTypeTraits(
    const Idl* idl,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict)
{
    idl->root.nodes.traverse(BindingTraitsMaker(rootDict, idl, importMaker));
}

void generateGuidRegistrations(
    const Idl* idl,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict)
{
    idl->root.nodes.traverse(
        GuidRegistrationMaker(rootDict, idl, importMaker));
}

/**
 * Generates the file, and returns its contents.
 */
void generateFile(const Idl* idl, bool isHeader, const utils::Path& prefix, std::vector<OutputFile>& files)
{
    tpl::DirGuard guard(isHeader ? "cpp_header" : "cpp_source");

    ctemplate::TemplateDictionary rootDict("ROOT");

    rootDict.SetValue(
        "RUNTIME_NAMESPACE_PREFIX",
        "::" + idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

    TypeNameMaker typeNameMaker;
    ImportMaker importMaker(idl, &typeNameMaker);

    if (isHeader) {
        generateTypeTraits(idl, &importMaker, &rootDict);
        generateBitfieldOperators(idl, &typeNameMaker, &rootDict);
        generateWeakRefCreatePlatforms(idl, &typeNameMaker, &importMaker, &rootDict);
    } else {
        generateGuidRegistrations(idl, &importMaker, &rootDict);
    }
    generateTypeDeclarations(
        idl, &typeNameMaker, &importMaker, &rootDict, isHeader);
    generateExternalSerialization(
        idl, &typeNameMaker, &importMaker, &rootDict, isHeader);

    importMaker.fill(filePath(idl, true).inAngles(), &rootDict);
    if (!isHeader) {
        rootDict.SetValue(
            "OWN_HEADER_INCLUDE_PATH", filePath(idl, true).asString());
    }

    common::setNamespace(idl->cppNamespace, &rootDict);

    files.emplace_back(
        prefix, filePath(idl, isHeader),
        tpl::expand("file.tpl", &rootDict),
        importMaker.getImportPaths()
    );
}

} // namespace

std::vector<OutputFile> generate(const Idl* idl)
{
    const auto& config = idl->env->config;

    TargetsFinder finder(idl->root);

    std::vector<OutputFile> files;
    if (finder.headerWillBeGenerated()) {
        generateFile(idl, true, config.outBaseHeadersRoot, files);
    }
    if (finder.sourceWillBeGenerated()) {
        generateFile(idl, false, config.outBaseImplRoot, files);
    }
    return files;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
