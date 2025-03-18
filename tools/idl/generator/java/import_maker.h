#pragma once

#include "common/import_maker.h"
#include "common/type_name_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/idl.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

class ImportMaker : public common::ImportMaker {
public:
    ImportMaker(
        const Idl* idl,
        const common::TypeNameMaker* typeNameMaker,
        bool isBinding);

    virtual void addAll(
        const FullTypeRef& typeRef,
        bool withSerialization,
        bool isHeader = false) override;

private:
    bool isBinding_;
};

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
