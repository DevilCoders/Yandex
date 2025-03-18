#pragma once

#include "common/import_maker.h"
#include "common/type_name_maker.h"
#include "common/struct_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

/**
 * Fills "serialize" function's dictionary. Only structs need custom code for
 * serialization.
 */
void addStructSerialization(
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* serializationDict,
    const FullScope& scope,
    const nodes::Struct& s,
    bool isHeader);

/**
 * Traverses all nodes in .idl file, and adds dictionary with external
 * serialization to given root dictionary, if needed. And it is needed only
 * for non-options structs.
 */
class ExtSerialStructMaker : public common::StructMaker {
public:
    ExtSerialStructMaker(
        const common::TypeNameMaker* typeNameMaker,
        common::ImportMaker* importMaker,
        ctemplate::TemplateDictionary* rootDict,
        bool isHeader);

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s) override;

private:
    ctemplate::TemplateDictionary* rootDict_;
    ctemplate::TemplateDictionary* dict_{ nullptr };

    const bool isHeader_;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
