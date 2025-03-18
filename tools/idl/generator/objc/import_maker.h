#pragma once

#include "common/import_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/idl.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

class ImportMaker : public common::ImportMaker {
public:
    ImportMaker(
        const Idl* idl,
        const common::TypeNameMaker* typeNameMaker,
        bool isHeader);

    virtual void addAll(
        const FullTypeRef& typeRef,
        bool withSerialization,
        bool isHeader = false) override;

    virtual void fill(
        const std::string& selfImportPath,
        ctemplate::TemplateDictionary* dict) override;

    virtual bool addForwardDeclaration(
        const FullTypeRef& typeRef,
        const std::string& importPath) override;

private:
    const bool isHeader_;
};

void addRuntimeImportPath(
    common::ImportMaker* importMaker,
    const std::string& importPath);

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
