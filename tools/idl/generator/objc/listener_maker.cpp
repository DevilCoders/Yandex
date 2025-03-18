#include "objc/listener_maker.h"

#include <yandex/maps/idl/utils.h>
#include "tpl/tpl.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

ListenerMaker::ListenerMaker(
    const common::TypeNameMaker* typeNameMaker,
    const common::DocMaker* docMaker,
    common::FunctionMaker* functionMaker)
    : common::ListenerMaker(typeNameMaker, docMaker, functionMaker, true)
{
}

ctemplate::TemplateDictionary* ListenerMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Listener& l)
{
    if (!needGenerateNode(scope, l))
        return parentDict;
    return common::ListenerMaker::make(parentDict, scope, l);
}

ctemplate::TemplateDictionary* ListenerMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/,
    const nodes::Listener& l) const
{
    const auto tplName = l.isLambda && isLambdaSupported_ ?
        "lambda_listener.tpl" : "listener.tpl";
    return tpl::addSectionedInclude(parentDict, "NON_FWD_DECL_ABLE_TYPE", tplName);
}

ctemplate::TemplateDictionary* ListenerMaker::makeFunction(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Listener& l,
    const nodes::Function& f)
{
    if (l.isLambda && f.parameters.empty()) {
        if (l.functions.size() == 1)
            // single method without parameters
            parentDict->ShowSection("VOID_EMPTY");
        return nullptr;
    }

    auto dict = common::ListenerMaker::makeFunction(parentDict, scope, l, f);
    if (l.isLambda && isLambdaSupported_) {
        if (l.functions.size() > 1) {
            dict->ShowSection("MERGED");
        } else {
            dict->ShowSection("NON_MERGED");
        }
    }

    return dict;
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
