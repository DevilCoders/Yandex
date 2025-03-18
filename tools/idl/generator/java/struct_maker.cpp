#include "java/struct_maker.h"

#include "common/common.h"
#include "common/import_maker.h"
#include "common/type_name_maker.h"
#include "cpp/type_name_maker.h"
#include "java/annotation_addition.h"
#include "java/import_maker.h"
#include "java/serialization_addition.h"

#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

ctemplate::TemplateDictionary* StructMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Struct& s)
{
    auto dict = common::StructMaker::make(parentDict, scope, s);

    if (s.kind == nodes::StructKind::Bridged) {
        importMaker_->addJavaRuntimeImportPath("NativeObject");
        importMaker_->addImportPath("java.nio.ByteBuffer");
    }

    auto isInterface = [scope] (const nodes::StructField& f) {
        return FullTypeRef(scope, f.typeRef).is<nodes::Interface>();
    };

    if (s.kind != nodes::StructKind::Options || !s.nodes.count<nodes::StructField>(isInterface)) {
        addStructSerialization(typeNameMaker_, importMaker_, dict, scope, s);
    }

    return dict;
}

void StructMaker::fillFieldDict(
    ctemplate::TemplateDictionary* dict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    common::StructMaker::fillFieldDict(dict, scope, s, f);

    FullTypeRef typeRef(scope, f.typeRef);
    addNullabilityAnnotation(dict, importMaker_, typeRef);
}

ctemplate::TemplateDictionary* StructMaker::makeField(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Struct& s,
        const nodes::StructField& f)
{
    if (!needGenerateNode(scope, f))
        return parentDict;
    return common::StructMaker::makeField(parentDict, scope, s, f);
}

bool StructMaker::addConstructorField(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::StructField& f,
    unsigned int fieldsNumber)
{
    if (!needGenerateNode(scope, f))
        return false;
    return common::StructMaker::addConstructorField(parentDict, scope, f, fieldsNumber);
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
