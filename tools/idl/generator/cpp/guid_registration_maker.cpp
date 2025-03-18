#include "cpp/guid_registration_maker.h"

#include "common/common.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"

#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/idl.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

GuidRegistrationMaker::GuidRegistrationMaker(
    ctemplate::TemplateDictionary* rootDict,
    const Idl* idl,
    common::ImportMaker* importMaker)
    : rootDict_(rootDict),
      idl_(idl),
      importMaker_(importMaker)
{
}

void GuidRegistrationMaker::onVisited(const nodes::Interface& i)
{
    traverseWithScope(scope_, i, this);
}

void GuidRegistrationMaker::onVisited(const nodes::Struct& s)
{
    if (s.kind == nodes::StructKind::Bridged) {
        importMaker_->addCppRuntimeImportPath("any/guid_registration.h");

        const auto& typeInfo = idl_->type(scope_, Scope(s.name.original()));
        auto fullName = typeInfo.fullNameAsScope[CPP].asString("::");
        rootDict_->SetValueAndShowSection(
            "STRUCT_FULL_NAME", fullName, "GUID_REGISTRATION");
    }

    traverseWithScope(scope_, s, this);
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
