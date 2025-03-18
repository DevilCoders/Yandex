#pragma once

#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

void addStructSerialization(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s);

void addFieldSerialization(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* dict,
    const FullTypeRef& typeRef);

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
