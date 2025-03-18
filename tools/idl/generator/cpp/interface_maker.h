#pragma once

#include "common/interface_maker.h"

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

class InterfaceMaker : public common::InterfaceMaker {
public:
    using common::InterfaceMaker::InterfaceMaker;

    virtual ctemplate::TemplateDictionary* make(
        ctemplate::TemplateDictionary* parentDict,
        FullScope scope,
        const nodes::Interface& i) override;
};

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
