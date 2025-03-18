#include "common/enum_maker.h"

#include "common/common.h"
#include "common/doc_maker.h"
#include "common/enum_field_maker.h"
#include "common/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>

#include <boost/regex.hpp>

#include <set>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

EnumMaker::EnumMaker(
    const TypeNameMaker* typeNameMaker,
    const EnumFieldMaker* fieldMaker,
    const DocMaker* docMaker)
    : typeNameMaker_(typeNameMaker),
      fieldMaker_(fieldMaker),
      docMaker_(docMaker)
{
}

EnumMaker::~EnumMaker()
{
}

ctemplate::TemplateDictionary* EnumMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Enum& e)
{
    auto dict = createDict(parentDict, scope);

    if (isExcludeDoc(e.doc)) {
        dict->ShowSection("EXCLUDE");
    }

    if (docMaker_) {
        docMaker_->make(dict, "DOCS", scope, e.doc);
    }

    FullTypeRef typeRef(scope, e.name);
    dict->SetValue("CONSTRUCTOR_NAME",
        typeNameMaker_->makeConstructorName(typeRef.info()));
    dict->SetValue("INSTANCE_NAME",
        typeNameMaker_->makeInstanceName(typeRef.info()));
    dict->SetValue("TYPE_NAME", typeNameMaker_->make(typeRef));
    dict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));

    for (const auto& field : e.fields) {
        auto fieldDict = dict->AddSectionDictionary("ENUM_FIELD");

        if (docMaker_) {
            docMaker_->make(fieldDict, "FIELD_DOCS", scope, field.doc);
        }
        if (isExcludeDoc(field.doc)) {
            fieldDict->ShowSection("EXCLUDE_FIELD");
        }
        if (&field == &(e.fields.back())) {
            fieldDict->ShowSection("LAST_ENUM_FIELD");
        }

        fieldDict->SetValue("FIELD_NAME",
            fieldMaker_->makeDefinition(typeRef, field.name));
        if (field.value) {
            fieldDict->SetValueAndShowSection("VALUE",
                fieldValue(scope, e, *field.value), "VALUE");
        }
    }

    dict->ShowSection(e.isBitField ? "BITFIELD" : "SIMPLE_ENUM");
    dict->ShowSection(scope.scope.isEmpty() ? "TOP_LEVEL" : "INNER_LEVEL");

    return dict;
}

ctemplate::TemplateDictionary* EnumMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/) const
{
    return tpl::addSectionedInclude(parentDict, "CHILD", "enum.tpl");
}

std::string EnumMaker::fieldValue(
    const FullScope& scope,
    const nodes::Enum& e,
    const std::string& idlFieldValue) const
{
    std::set<std::string> enumFieldNames;
    for (const auto& field : e.fields) {
        enumFieldNames.insert(field.name);
    }

    FullTypeRef typeRef(scope, e.name);
    return boost::regex_replace(idlFieldValue, boost::regex("\\w+"),
        [&](boost::smatch match)
        {
            if (enumFieldNames.find(match[0]) != enumFieldNames.end()) {
                return fieldMaker_->makeValue(typeRef, match[0], true);
            } else {
                return std::string(match[0]);
            }
        }
    );
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
