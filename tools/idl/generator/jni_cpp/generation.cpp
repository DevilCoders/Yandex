#include <yandex/maps/idl/generator/jni_cpp/generation.h>

#include "common/common.h"
#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/generator.h"
#include "common/pod_decider.h"
#include "cpp/common.h"
#include "cpp/enum_field_maker.h"
#include "cpp/interface_maker.h"
#include "cpp/type_name_maker.h"
#include "jni_cpp/common.h"
#include "jni_cpp/function_maker.h"
#include "jni_cpp/import_maker.h"
#include "jni_cpp/interface_maker.h"
#include "jni_cpp/listener_maker.h"
#include "jni_cpp/struct_maker.h"
#include "jni_cpp/variant_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>

#include <ctemplate/template.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

namespace {

/**
 * Tells whether a header will be generated for given .idl root or no.
 */
class GeneratedTargetsFinder : public nodes::Visitor {
public:
    GeneratedTargetsFinder(const nodes::Root& root)
    {
        root.nodes.traverse(this);
    }

    bool headerWillBeGenerated() const { return headerWillBeGenerated_; }
    bool sourceWillBeGenerated() const { return sourceWillBeGenerated_; }

    bool willCreatePublicCode() const { return willCreatePublicCode_; }

private:
    bool headerWillBeGenerated_{ false };
    bool sourceWillBeGenerated_{ false };

    /**
     * Bindings for structs and listeners (lambda or not) create
     * code that can be called from other places (public code).
     */
    bool willCreatePublicCode_{ false };

    void onVisited(const nodes::Interface& i) override
    {
        sourceWillBeGenerated_ = true;
        i.nodes.traverse(this);
    }
    void onVisited(const nodes::Listener& /* l */) override
    {
        headerWillBeGenerated_ = sourceWillBeGenerated_ = true;
        willCreatePublicCode_ = true;
    }
    void onVisited(const nodes::Struct& s) override
    {
        headerWillBeGenerated_ = sourceWillBeGenerated_ = true;
        willCreatePublicCode_ = true;
        s.nodes.traverse(this);
    }
};

void generateFile(
    const GeneratedTargetsFinder& finder,
    const Idl* idl,
    bool isHeader,
    const utils::Path& prefixPath,
    std::vector<OutputFile>& files)
{
    tpl::DirGuard guard("jni_cpp");

    ctemplate::TemplateDictionary rootDict("ROOT");

    rootDict.SetValue("RUNTIME_NAMESPACE_PREFIX",
        "::" + idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

    // Create maker inputs
    common::PodDecider podDecider;
    cpp::TypeNameMaker typeNameMaker;
    ImportMaker importMaker(idl, &typeNameMaker);
    cpp::EnumFieldMaker enumFieldMaker(&typeNameMaker);
    FunctionMaker functionMaker(
        &podDecider, &typeNameMaker, &importMaker, false);
    FunctionMaker listenerFunctionMaker(
        &podDecider, &typeNameMaker, &importMaker, false);

    // Create makers
    InterfaceMaker i(&podDecider, &typeNameMaker, &importMaker,
        &functionMaker, isHeader, &rootDict);
    ListenerMaker l(&typeNameMaker, &importMaker,
        &listenerFunctionMaker, &rootDict);
    StructMaker s(&podDecider, &typeNameMaker, &enumFieldMaker,
        &importMaker, &rootDict, isHeader);
    VariantMaker v(&podDecider, &typeNameMaker, &importMaker, &rootDict);

    // Create base dictionary
    common::Generator gen(&rootDict, { nullptr, &i, &l, &s, &v }, idl);
    idl->root.nodes.traverse(&gen);

    common::setNamespace(idl->cppNamespace, &rootDict);

    // Add includes
    importMaker.addImportPath(cpp::filePath(idl, true).inAngles());

    importMaker.fill(filePath(idl, true).inAngles(), &rootDict);

    // Add public methods
    if (finder.willCreatePublicCode()) {
        rootDict.ShowSection("PUBLIC_METHODS");
    }

    // Add utility code
    auto needToPlatformForStrongRefListener =
        i.generatedStrongRefListenerProperty() ||
        functionMaker.generatedStrongRefListenerReturnValue() ||
        listenerFunctionMaker.generatedStrongRefListenerParameter();
    auto needConditionalMakeShared =
        listenerFunctionMaker.generatedListenerReturnValue();
    if (needToPlatformForStrongRefListener || needConditionalMakeShared) {
        auto utilDict = rootDict.AddSectionDictionary("NEED_UNNAMED_NAMESPACE");

        if (needToPlatformForStrongRefListener) {
            utilDict->ShowSection("NEED_TO_PLATFORM_FOR_STRONG_REF_LISTENER");
        }
        if (needConditionalMakeShared) {
            utilDict->ShowSection("NEED_CONDITIONAL_MAKE_SHARED");
        }
    }

    // Apply final changes
    rootDict.ShowSection(finder.headerWillBeGenerated() ?
        "HEADER_WILL_BE_GENERATED" : "HEADER_WILL_NOT_BE_GENERATED");
    rootDict.SetValue(
        "OWN_HEADER_INCLUDE_PATH", filePath(idl, true).asString());

    // Generate!
    files.emplace_back(
        prefixPath, filePath(idl, isHeader),
        tpl::expand(isHeader ? "header.tpl" : "source.tpl", &rootDict),
        importMaker.getImportPaths()
    );
}

} // namespace

std::vector<OutputFile> generate(const Idl* idl)
{
    const auto& config = idl->env->config;

    GeneratedTargetsFinder finder(idl->root);

    std::vector<OutputFile> files;
    if (finder.headerWillBeGenerated()) {
        generateFile(finder, idl, true, config.outAndroidHeadersRoot, files);
    }
    if (finder.sourceWillBeGenerated()) {
        generateFile(finder, idl, false, config.outAndroidImplRoot, files);
    }
    return files;
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
