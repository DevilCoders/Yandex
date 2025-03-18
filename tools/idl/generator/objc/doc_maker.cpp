#include "objc/doc_maker.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

std::string DocMaker::optionalFieldAnnotation() const
{
    return "Optional field, can be nil.";
}

std::string DocMaker::optionalPropertyAnnotation() const
{
    return "Optional property, can be nil.";
}

std::string DocMaker::docExclusionAnnotation() const
{
    return ":nodoc:";
}

std::string DocMaker::emptyDocAnnotation() const
{
    return "Undocumented";
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
