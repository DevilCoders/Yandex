#pragma once

#include "cpp/import_maker.h"

#include <yandex/maps/idl/full_type_ref.h>

#include <ctemplate/template.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

class ImportMaker : public cpp::ImportMaker {
public:
    using cpp::ImportMaker::ImportMaker;

    virtual void addAll(
        const FullTypeRef& typeRef,
        bool withSerialization,
        bool isHeader = false) override;

    virtual void fill(
        const std::string& selfImportPath,
        ctemplate::TemplateDictionary* parentDict) override;
};

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
