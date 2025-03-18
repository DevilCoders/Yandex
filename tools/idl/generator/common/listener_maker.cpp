#include "common/listener_maker.h"

#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/type_name_maker.h"
#include "cpp/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

ListenerMaker::ListenerMaker(
    const TypeNameMaker* typeNameMaker,
    const DocMaker* docMaker,
    FunctionMaker* functionMaker,
    bool isLambdaSupported)
    : typeNameMaker_(typeNameMaker),
      docMaker_(docMaker),
      functionMaker_(functionMaker),
      isLambdaSupported_(isLambdaSupported)
{
}

ctemplate::TemplateDictionary* ListenerMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Listener& l)
{
    auto dict = createDict(parentDict, scope, l);

    dict->ShowSection(l.isStrongRef ? "STRONG_LISTENER" : "WEAK_LISTENER");
    if (isExcludeDoc(l.doc)) {
        dict->ShowSection("EXCLUDE");
    }

    if (docMaker_) {
        docMaker_->make(dict, "DOCS", scope, l.doc);
    }
    dict->ShowSection(scope.scope.isEmpty() ? "TOP_LEVEL" : "INNER_LEVEL");

    FullTypeRef typeRef(scope, l.name);
    dict->SetValue("CONSTRUCTOR_NAME",
        typeNameMaker_->makeConstructorName(typeRef.info()));
    dict->SetValue("INSTANCE_NAME",
        typeNameMaker_->makeInstanceName(typeRef.info()));
    dict->SetValue("TYPE_NAME", typeNameMaker_->make(typeRef));
    dict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));

    dict->SetValue("CPP_TYPE_NAME", cpp::fullName(typeRef));

    if (l.base) {
        dict->SetValueAndShowSection("PARENT_CLASS",
            typeNameMaker_->make(FullTypeRef(scope, *l.base)), "BASE_LISTENER");
    }

    ScopeGuard guard(&scope.scope, l.name.original());
    for (const auto& function : l.functions) {
        makeFunction(dict, scope, l, function);
    }

    return dict;
}

ctemplate::TemplateDictionary* ListenerMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/,
    const nodes::Listener& l) const
{
    const auto tplName = l.isLambda && isLambdaSupported_ ?
        "lambda_listener.tpl" : "listener.tpl";
    return tpl::addSectionedInclude(parentDict, "CHILD", tplName);
}

ctemplate::TemplateDictionary* ListenerMaker::makeFunction(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Listener& l,
    const nodes::Function& f)
{
    auto dict = functionMaker_->make(
        parentDict, scope, f, l.isLambda && isLambdaSupported_);
    if (docMaker_) {
        docMaker_->make(dict, "DOCS", scope, f);
    }

    dict->ShowSection("IS_NOT_STATIC");

    if (f.isConst) {
        dict->ShowSection("CONST_METHOD");
    }

    switch (f.threadRestriction) {
        case nodes::Function::ThreadRestriction::Ui:
            dict->ShowSection("UI_THREAD");
            break;
        case nodes::Function::ThreadRestriction::Bg:
            dict->ShowSection("BG_THREAD");
            break;
        case nodes::Function::ThreadRestriction::None:
            dict->ShowSection("ANY_THREAD");
            break;
    }

    return dict;
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
