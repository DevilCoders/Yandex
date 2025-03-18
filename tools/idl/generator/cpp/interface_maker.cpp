#include "cpp/interface_maker.h"

#include "common/common.h"
#include "common/import_maker.h"
#include "cpp/common.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

ctemplate::TemplateDictionary* InterfaceMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Interface& i)
{
    auto dict = common::InterfaceMaker::make(parentDict, scope, i);

    const auto& runtimeNamespace =
        scope.idl->env->runtimeFramework()->cppNamespace;

    if (i.isViewDelegate) {
        importMaker_->addCppRuntimeImportPath("view/view_delegate.h");

        dict->ShowSection("HAS_PARENT");

        auto parentName = fullName(runtimeNamespace + "view" + "ViewDelegate");
        dict->SetValueAndShowSection("NAME", parentName, "PARENT");
    }

    if (i.ownership == nodes::Interface::Ownership::Weak) {
        importMaker_->addImportPath("<boost/any.hpp>");
        importMaker_->addImportPath("<memory>");

        if (!i.base) {
            importMaker_->addCppRuntimeImportPath("platform_holder.h");

            dict->ShowSection("HAS_PARENT");

            auto parentName = fullName(
                runtimeNamespace + ("PlatformHolder<" + i.name[CPP] + ">"));
            dict->SetValueAndShowSection("NAME", parentName, "PARENT");
        }
    }

    return dict;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
