#include "objc/interface_maker.h"

#include "objc/common.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils/paths.h>
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

InterfaceMaker::InterfaceMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    common::FunctionMaker* functionMaker,
    bool isHeader,
    ctemplate::TemplateDictionary* rootDict)
    : common::InterfaceMaker(
          podDecider,
          typeNameMaker,
          importMaker,
          docMaker,
          functionMaker,
          isHeader),
      rootDict_(rootDict)
{
}

ctemplate::TemplateDictionary* InterfaceMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Interface& i)
{
    if (!needGenerateNode(scope, i))
        return parentDict;

    FullTypeRef typeRef(scope, i.name);

    if (scope.scope.size() > 0) {
        importMaker_->addForwardDeclaration(
            typeRef, filePath(scope.idl, true));
    }

    auto dict = common::InterfaceMaker::make(rootDict_, scope, i);

    if (i.base) {
        const auto& typeInfo = scope.type(*i.base);
        importMaker_->addImportPath(filePath(typeInfo.idl, true).inAngles());
    } else {
        if (i.isStatic && i.name.isDefined(OBJC)) {
            dict->SetValueAndShowSection(
                "CATEGORY", i.name.original(), "IS_CATEGORY");

            importMaker_->addImportPath(filePath(scope.idl, true).inAngles());
        } else {
            dict->ShowSection("HAS_PARENT");
            dict->SetValueAndShowSection("NAME", "NSObject", "PARENT");
        }
    }

    return dict;
}

ctemplate::TemplateDictionary* InterfaceMaker::makeProperty(
        ctemplate::TemplateDictionary* parentDict,
        const FullScope& scope,
        const nodes::Property& p,
        bool /* isBaseItem */)
{
    if (!needGenerateNode(scope, p))
        return parentDict;

    FullTypeRef typeRef(scope, p.typeRef);
    auto dict = common::InterfaceMaker::makeProperty(parentDict, scope, p);

    if (supportsOptionalAnnotation(typeRef)) {
        dict->ShowSection(hasNullableAnnotation(typeRef) ? "OPTIONAL" : "NON_OPTIONAL");
    }

    return dict;
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
