#include "java/listener_maker.h"

#include "java/annotation_addition.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

ListenerMaker::ListenerMaker(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    common::FunctionMaker* functionMaker)
    : common::ListenerMaker(typeNameMaker, docMaker, functionMaker, false),
      importMaker_(importMaker)
{
}

ctemplate::TemplateDictionary* ListenerMaker::makeFunction(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Listener& l,
    const nodes::Function& f)
{
    auto dict = common::ListenerMaker::makeFunction(parentDict, scope, l, f);

    addThreadRestrictionAnnotationImport(importMaker_, f.threadRestriction);

    return dict;
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
