#include <yandex/maps/idl/generator/obj_cpp/generation.h>

#include "common/common.h"
#include "common/generator.h"
#include "cpp/enum_field_maker.h"
#include "cpp/type_name_maker.h"
#include "obj_cpp/common.h"
#include "obj_cpp/function_maker.h"
#include "obj_cpp/import_maker.h"
#include "obj_cpp/interface_maker.h"
#include "obj_cpp/listener_maker.h"
#include "obj_cpp/struct_maker.h"
#include "obj_cpp/variant_maker.h"
#include "objc/common.h"
#include "objc/doc_maker.h"
#include "objc/pod_decider.h"
#include "objc/enum_field_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>

#include <ctemplate/template_cache.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <string>
#include <optional>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

namespace {

class CustomImplFileFilter : public common::Filter<true> {
public:
    bool isUseful(const Scope& /*scope*/, const nodes::Struct& s) const override
    {
        return !s.customCodeLink.baseHeader;
    }
};

void generateUnnamedNamespaceSection(
    ctemplate::TemplateDictionary* parentDictionary,
    const InterfaceMaker& interfaceMaker,
    const FunctionMaker& functionMaker,
    const ListenerMaker& listenerMaker,
    const FunctionMaker& listenerFunctionMaker)
{
    auto needToPlatformForStrongRefListener =
        interfaceMaker.visitedStrongRefListenerProperty() ||
        (interfaceMaker.visited() &&
            functionMaker.visitedStrongRefListenerReturnValue()) ||
        (listenerMaker.visited() &&
            listenerFunctionMaker.visitedStrongRefListenerParameter());

    auto needConditionalMakeShared = listenerMaker.visited() &&
        listenerFunctionMaker.visitedListenerReturnValue();
    if (needToPlatformForStrongRefListener || needConditionalMakeShared) {
        auto utilDict =
            parentDictionary->AddSectionDictionary("NEED_UNNAMED_NAMESPACE");

        if (needToPlatformForStrongRefListener) {
            utilDict->ShowSection("NEED_TO_PLATFORM_FOR_STRONG_REF_LISTENER");
        }
        if (needConditionalMakeShared) {
            utilDict->ShowSection("NEED_CONDITIONAL_MAKE_SHARED");
        }
    }
}

bool shouldGenerateFile (
    bool isHeader,
    const InterfaceMaker& i,
    const ListenerMaker& l,
    const StructMaker& s,
    const VariantMaker& v)
{
    if (i.visitedNotStatic() ||
            l.visited() || s.visited() || v.visited()) {
        return true;
    }
    if (!isHeader && i.visited()) {
        return true;
    }
    return false;
}

std::optional<OutputFile> generateIdlCorrespondingFile(
    const Idl* idl,
    const std::string& tplDir,
    const utils::Path& outRootPath,
    const utils::Path& outRelativePath,
    bool isHeader)
{
    tpl::DirGuard guard(tplDir);

    ctemplate::TemplateDictionary rootDict("ROOT");

    rootDict.SetValue("RUNTIME_NAMESPACE_PREFIX",
        "::" + idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

    objc::PodDecider podDecider;
    cpp::TypeNameMaker typeNameMaker;
    ImportMaker importMaker(idl, &typeNameMaker, isHeader);
    cpp::EnumFieldMaker enumFieldMaker(&typeNameMaker);
    FunctionMaker functionMaker(
        &podDecider, &typeNameMaker, &importMaker, isHeader);
    FunctionMaker listenerFunctionMaker(
        &podDecider, &typeNameMaker, &importMaker, isHeader);

    InterfaceMaker interfaceMaker(&podDecider, &typeNameMaker, &importMaker,
        nullptr, &functionMaker, isHeader, &rootDict);
    ListenerMaker listenerMaker(&typeNameMaker, nullptr,
        &listenerFunctionMaker, &rootDict, isHeader, &importMaker);
    StructMaker structMaker(&podDecider, &typeNameMaker, &enumFieldMaker,
        &importMaker, nullptr, &rootDict, isHeader);
    VariantMaker variantMaker(&podDecider, &typeNameMaker, &importMaker, nullptr, &rootDict);

    std::shared_ptr<common::AbstractFilter> filter = isHeader ? nullptr : std::make_shared<CustomImplFileFilter>();
    common::Generator g(&rootDict, {nullptr, &interfaceMaker, &listenerMaker, &structMaker, &variantMaker}, idl, filter.get());
    idl->root.nodes.traverse(&g);

    if (!shouldGenerateFile(isHeader, interfaceMaker, listenerMaker, structMaker, variantMaker)) {
        return std::nullopt;
    }

    if (listenerMaker.visited()) {
        common::setNamespace(idl->cppNamespace + "ios",
            rootDict.AddSectionDictionary("NAMESPACE_SECTION"));
    }

    generateUnnamedNamespaceSection(
        &rootDict, interfaceMaker, functionMaker, listenerMaker, listenerFunctionMaker);

    importMaker.fill(outRelativePath.inAngles(), &rootDict);
    return {{outRootPath, outRelativePath,
        objc::alignParameters(tpl::expand("file.tpl", &rootDict)),
        importMaker.getImportPaths()}};
}

} // namespace

std::vector<OutputFile> generate(const Idl* idl)
{

    const auto& config = idl->env->config;

    auto header = generateIdlCorrespondingFile(idl, "obj_cpp_header", config.outIosHeadersRoot, filePath(idl, true), true);
    auto source = generateIdlCorrespondingFile(idl, "obj_cpp_source", config.outIosImplRoot, filePath(idl, false), false);
    std::vector<OutputFile> files;
    if (header)
        files.push_back(header.value());
    if (source)
        files.push_back(source.value());
    return files;
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
