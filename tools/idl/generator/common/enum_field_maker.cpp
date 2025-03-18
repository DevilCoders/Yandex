#include "common/enum_field_maker.h"

#include "common/type_name_maker.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

EnumFieldMaker::EnumFieldMaker(
    const TypeNameMaker* typeNameMaker)
    : typeNameMaker_(typeNameMaker)
{
}

EnumFieldMaker::~EnumFieldMaker()
{
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
