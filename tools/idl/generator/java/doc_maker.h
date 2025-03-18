#pragma once

#include "common/doc_maker.h"

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

class DocMaker : public common::DocMaker {
public:
    using common::DocMaker::DocMaker;

    virtual std::string optionalFieldAnnotation() const override;
    virtual std::string optionalPropertyAnnotation() const override;
    virtual std::string docExclusionAnnotation() const override;
    virtual std::string emptyDocAnnotation() const override;

};

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
