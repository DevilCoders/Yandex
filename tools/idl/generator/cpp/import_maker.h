#pragma once

#include "common/import_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/type_info.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

class ImportMaker : public common::ImportMaker {
public:
    ImportMaker(const Idl* idl, const common::TypeNameMaker* typeNameMaker);

    virtual void addAll(
        const FullTypeRef& typeRef,
        bool withSerialization,
        bool dontImportIfPossible = false) override;

    using common::ImportMaker::addForwardDeclaration;

    virtual bool addForwardDeclaration(
        const FullTypeRef& typeRef,
        const std::string& importPath) override;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
