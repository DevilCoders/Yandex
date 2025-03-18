#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class DocMaker;
class EnumFieldMaker;
class ImportMaker;
class PodDecider;
class TypeNameMaker;

class StructMaker {
public:
    StructMaker(
        const PodDecider* podDecider,
        const TypeNameMaker* typeNameMaker,
        const EnumFieldMaker* enumFieldMaker,
        ImportMaker* importMaker,
        const DocMaker* docMaker);

    virtual ~StructMaker();

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s);

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s) const;

    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f);

    /**
     * Creates struct field's target-specific default value.
     */
    virtual std::string fieldValue(
        const FullScope& scope,
        const nodes::StructField& f) const;

    virtual void fillFieldDict(
        ctemplate::TemplateDictionary* dict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f);

    virtual void addConstructor(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s);

    virtual bool addConstructorField(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::StructField& f,
        unsigned int fieldsNumber);

    std::string fillHiddenFieldValue(
        const FullScope& scope, 
        const nodes::StructField& f);

    ctemplate::TemplateDictionary* createDictForInternalStruct(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Struct& s);

    const PodDecider* podDecider_;

    const TypeNameMaker* typeNameMaker_;
    const EnumFieldMaker* enumFieldMaker_;
    ImportMaker* importMaker_;

    const DocMaker* docMaker_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
