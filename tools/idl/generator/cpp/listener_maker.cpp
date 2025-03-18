#include "cpp/listener_maker.h"

#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

ListenerMaker::ListenerMaker(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    common::FunctionMaker* functionMaker)
    : common::ListenerMaker(typeNameMaker, docMaker, functionMaker, true),
      importMaker_(importMaker)
{
}

ctemplate::TemplateDictionary* ListenerMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Listener& l)
{
    if (l.isLambda) {
        importMaker_->addImportPath("<functional>");
    }

    return common::ListenerMaker::make(parentDict, scope, l);
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
