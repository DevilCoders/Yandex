#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/nodes/variant.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class DocMaker;
class ImportMaker;
class PodDecider;
class TypeNameMaker;

class VariantMaker {
public:
    VariantMaker(
        const PodDecider* podDecider,
        const TypeNameMaker* typeNameMaker,
        ImportMaker* importMaker,
        const DocMaker* docMaker);

    virtual ~VariantMaker();

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Variant& v);

protected:
    virtual ctemplate::TemplateDictionary* createDict(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope) const;

    virtual ctemplate::TemplateDictionary* makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Variant& v,
        const nodes::VariantField& f,
        int fieldIndex) const;

    virtual std::string fieldName(
        const FullScope& scope,
        const nodes::Variant& v,
        const nodes::TypeRef& fieldTypeRef,
        bool isOptional = false) const;

    const PodDecider* podDecider_;
    const TypeNameMaker* typeNameMaker_;
    ImportMaker* importMaker_;

    const DocMaker* docMaker_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
