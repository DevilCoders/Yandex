#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class DocMaker;
class EnumFieldMaker;
class TypeNameMaker;

class EnumMaker {
public:
    EnumMaker(
        const TypeNameMaker* typeNameMaker,
        const EnumFieldMaker* fieldMaker,
        const DocMaker* docMaker);

    virtual ~EnumMaker();

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Enum& e);

protected:
    /**
     * Creates enum's dictionary.
     */
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope) const;

    /**
     * Creates enum field's target-specific value.
     */
    std::string fieldValue(
        const FullScope& scope,
        const nodes::Enum& e,
        const std::string& idlFieldValue) const;

protected:
    const TypeNameMaker* typeNameMaker_;
    const EnumFieldMaker* fieldMaker_;

    const DocMaker* docMaker_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
