#include <yandex/maps/idl/generator/objc/generation.h>

#include "common/common.h"
#include "common/generator.h"
#include "objc/common.h"
#include "objc/doc_maker.h"
#include "objc/enum_field_maker.h"
#include "objc/enum_maker.h"
#include "objc/function_maker.h"
#include "objc/import_maker.h"
#include "objc/interface_maker.h"
#include "objc/listener_maker.h"
#include "objc/pod_decider.h"
#include "objc/struct_maker.h"
#include "objc/type_name_maker.h"
#include "objc/variant_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils/paths.h>

#include <ctemplate/template.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

namespace {

OutputFile generateIdlCorrespondingFile(
    const Idl* idl,
    const std::string& tplDir,
    const utils::Path& outRootPath,
    const utils::Path& outRelativePath,
    bool isHeader)
{
    tpl::DirGuard guard(tplDir);

    ctemplate::TemplateDictionary rootDict("ROOT");

    PodDecider podDecider;
    TypeNameMaker typeNameMaker;
    ImportMaker importMaker(idl, &typeNameMaker, isHeader);
    EnumFieldMaker enumFieldMaker(&typeNameMaker);
    FunctionMaker functionMaker(
        &podDecider, &typeNameMaker, &importMaker, OBJC, isHeader);
    DocMaker docMaker("objc_docs", &typeNameMaker, &importMaker,
        &enumFieldMaker, &functionMaker);

    EnumMaker e(&typeNameMaker, &enumFieldMaker, &docMaker);
    InterfaceMaker i(&podDecider, &typeNameMaker,
        &importMaker, &docMaker, &functionMaker, isHeader, &rootDict);
    ListenerMaker l(&typeNameMaker, &docMaker, &functionMaker);
    StructMaker s(&podDecider, &typeNameMaker, &enumFieldMaker,
        &importMaker, &docMaker, &rootDict);
    VariantMaker v(&podDecider, &typeNameMaker, &importMaker, &docMaker, &rootDict);

    common::Generator g(&rootDict, { &e, &i, &l, &s, &v }, idl);
    idl->root.nodes.traverse(&g);

    common::setNamespace(idl->objcTypePrefix, &rootDict);

    if (!isHeader) {
        importMaker.addImportPath(
            outRelativePath.withExtension("h").inAngles());
    }
    importMaker.fill(outRelativePath.inAngles(), &rootDict);

    return {outRootPath, outRelativePath,
        alignParameters(tpl::expand("file.tpl", &rootDict)),
        importMaker.getImportPaths()};
}

} // namespace

std::vector<OutputFile> generate(const Idl* idl)
{
    const auto& config = idl->env->config;

    auto header = generateIdlCorrespondingFile(idl, "objc_header", config.outIosHeadersRoot, filePath(idl, true), true);
    auto source = generateIdlCorrespondingFile(idl, "objc_source", config.outIosImplRoot, filePath(idl, false), false);
    return {header, source};
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
