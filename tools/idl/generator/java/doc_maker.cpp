#include "java/doc_maker.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

std::string DocMaker::optionalFieldAnnotation() const
{
    return "Optional field, can be null.";
}

std::string DocMaker::optionalPropertyAnnotation() const
{
    return "Optional property, can be null.";
}

std::string DocMaker::docExclusionAnnotation() const
{
    return "@exclude";
}

std::string DocMaker::emptyDocAnnotation() const
{
    return "";
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
