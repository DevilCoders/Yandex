#pragma once

#include "common/interface_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

/**
 * Generates createPlatform methods for weak_ref interfaces.
 */
class CreatePlatformsInterfaceMaker : public common::InterfaceMaker {
public:
    CreatePlatformsInterfaceMaker(
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        ctemplate::TemplateDictionary* rootDict);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Interface& i) override;

private:
    ctemplate::TemplateDictionary* rootDict_;

    ctemplate::TemplateDictionary* dict_{ nullptr };
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
