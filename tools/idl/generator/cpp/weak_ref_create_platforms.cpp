#include "cpp/weak_ref_create_platforms.h"

#include "common/import_maker.h"

#include "tpl/tpl.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

CreatePlatformsInterfaceMaker::CreatePlatformsInterfaceMaker(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict)
    : common::InterfaceMaker(
          nullptr, typeNameMaker, importMaker, nullptr, nullptr, false),
      rootDict_(rootDict)
{
}

ctemplate::TemplateDictionary* CreatePlatformsInterfaceMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Interface& i)
{
    if (i.ownership == nodes::Interface::Ownership::Weak) {
        if (!dict_) {
            dict_ = tpl::addInclude(rootDict_,
                "WEAK_REF_CREATE_PLATFORMS", "weak_ref_create_platforms.tpl");
        }

        auto createPlatformDict =
            dict_->AddSectionDictionary("CREATE_PLATFORM");

        if (needGenerateNode(scope, i))
            createPlatformDict->ShowSection("TARGET_VISIBLE");

        importMaker_->addCppRuntimeImportPath("assert.h");

        FullTypeRef typeRef(scope, i.name);
        createPlatformDict->SetValue(
            "INSTANCE_NAME", typeNameMaker_->makeInstanceName(typeRef.info()));
        createPlatformDict->SetValue(
            "TYPE_NAME", typeNameMaker_->make(typeRef));
    }

    return dict_;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
