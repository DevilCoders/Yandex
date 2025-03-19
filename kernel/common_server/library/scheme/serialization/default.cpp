#include "default.h"

namespace NCS {
    namespace NScheme {
        NJson::TJsonValue TDefaultSerializer::SerializeToJsonImpl(const TStructureImpl& scheme) const {
            NJson::TJsonValue result;
            if (Traits & EReportTraits::TopLevelScheme) {
                ui32 idx = 1;
                for (auto&& i : scheme.GetElements()) {
                    Y_ASSERT(!!i->GetFieldName());
                    if (!!i->GetFieldName()) {
                        NJson::TJsonValue item = SerializeToJsonImpl(*i);
                        if (Traits & EReportTraits::OrderField) {
                            if (i->HasOrderIdx()) {
                                item.InsertValue("order", ++idx + i->GetOrderIdxUnsafe() * 1000);
                            } else {
                                item.InsertValue("order", ++idx);
                            }
                        }
                        JWRITE(result, i->GetFieldName(), std::move(item));
                    }
                }

            } else {
                result = SerializeToJsonImpl(scheme);
            }
            return result;
        }

        NJson::TJsonValue TDefaultSerializer::SerializeToJsonImpl(const IElement& element) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            if (Traits & EReportTraits::TypeField) {
                result.InsertValue("type", NUtil::ToCamelCase(::ToString(element.GetType())));
            }
            if (element.GetRegisterType() != element.GetType() && (Traits & EReportTraits::InternalTypeField)) {
                result.InsertValue("internalType", ::ToString(element.GetRegisterType()));
            }
            if (Traits & EReportTraits::TabNameField) {
                JWRITE_DEF(result, "tabName", element.GetTabName(), "default");
            }
            if (Traits & EReportTraits::ReadOnlyField) {
                result.InsertValue("readOnly", element.IsReadOnly());
            }
            result.InsertValue("readable", element.IsReadable());
            result.InsertValue("writable", element.IsWritable());
            if (!!element.HasDeprecated()) {
                result.InsertValue("deprecated", element.GetDeprecatedUnsafe());
            }
            if (!!element.GetDescription() && (Traits & EReportTraits::DescriptionField)) {
                result.InsertValue("displayName", element.GetDescription());
            }
            if (Traits & EReportTraits::RequiredField) {
                result.InsertValue("required", element.IsRequired());
            }
            if (!element.GetDefaultValueView().IsNull()) {
                result.InsertValue("default", element.GetDefaultValueView());
            }

            {
                auto stringImpl = dynamic_cast<const TFSString*>(&element);
                if (stringImpl) {
                    if (stringImpl->GetVisualType() != TFSString::EVisualType::Unknown) {
                        result.InsertValue("visual", ::ToString(stringImpl->GetVisualType()));
                    }
                    result.InsertValue("multiline", stringImpl->IsMultiLine());
                    result.InsertValue("nonEmpty", stringImpl->IsNonEmpty());
                }
            }

            {
                auto structureImpl = dynamic_cast<const TFSStructure*>(&element);
                if (structureImpl) {
                    Y_ASSERT(!!structureImpl->GetStructureElement());
                    if (!!structureImpl->GetStructureElement()) {
                        if (structureImpl->GetVisualType() != TFSStructure::EVisualType::Default) {
                            TJsonProcessor::WriteAsString(result, "visual", structureImpl->GetVisualType());
                        }
                        result.InsertValue("structure", SerializeToJsonImpl(*structureImpl->GetStructureElement()));
                    }
                }
            }

            {
                auto variantsImpl = dynamic_cast<const TFSVariants*>(&element);
                if (variantsImpl) {
                    NJson::TJsonValue& variantsJson = result.InsertValue("variants", NJson::JSON_ARRAY);
                    for (auto&& i : variantsImpl->GetVariants()) {
                        variantsJson.AppendValue(i);
                    }
                    result.InsertValue("editable", variantsImpl->GetEditable());
                    result.InsertValue("multiSelect", variantsImpl->GetMultiSelect());
                    if (variantsImpl->GetVisual()) {
                        result.InsertValue("visual", variantsImpl->GetVisual());
                    }
                    /*
                    if (NeedTranslate && Singleton<ICSOperator>()->HasServer()) {
                        NJson::TJsonValue& translatesJson = result.InsertValue("variants_translate", NJson::JSON_ARRAY);
                        for (auto&& i : Variants) {
                            translatesJson.AppendValue(Singleton<ICSOperator>()->GetServer().GetLocalization()->GetLocalString("rus", "variants." + i));
                        }
                    }
                    */
                }
            }

            {
                auto variantsImpl = dynamic_cast<const TFSWideVariants*>(&element);
                if (variantsImpl) {
                    NJson::TJsonValue& variantsJson = result.InsertValue("variants", NJson::JSON_ARRAY);
                    for (auto&& i : variantsImpl->GetVariants()) {
                        variantsJson.AppendValue(i.SerializeToJson());
                    }
                    if (variantsImpl->HasCustomStructureId() && !!variantsImpl->GetCustomStructureIdUnsafe()) {
                        result.InsertValue("customStructureId", variantsImpl->GetCustomStructureIdUnsafe());
                    } else {
                        result.InsertValue("customStructureId", NJson::JSON_NULL);
                    }
                    /*
                    if (NeedTranslate && Singleton<ICSOperator>()->HasServer()) {
                        NJson::TJsonValue& translatesJson = result.InsertValue("variants_translate", NJson::JSON_ARRAY);
                        for (auto&& i : Variants) {
                            translatesJson.AppendValue(Singleton<ICSOperator>()->GetServer().GetLocalization()->GetLocalString("rus", "variants." + i));
                        }
                    }
                    */
                }
            }

            {
                auto impl = dynamic_cast<const TFSArray*>(&element);
                if (impl) {
                    result.InsertValue("nonEmpty", impl->IsNonEmpty());
                    if (!!impl->HasMinItems()) {
                        result.InsertValue("minItems", impl->GetMinItemsUnsafe());
                    }
                    if (!!impl->HasMaxItems()) {
                        result.InsertValue("maxItems", impl->GetMaxItemsUnsafe());
                    }
                    Y_ASSERT(!!impl->GetArrayTypeElement());
                    if (!!impl->GetArrayTypeElement()) {
                        result.InsertValue("arrayType", SerializeToJsonImpl(*(impl->GetArrayTypeElement())));
                    }
                }
            }

            {
                auto impl = dynamic_cast<const TFSNumeric*>(&element);
                if (impl) {
                    if (!!impl->HasMin()) {
                        result.InsertValue("min", impl->GetMinUnsafe());
                    }
                    if (!!impl->HasMax()) {
                        result.InsertValue("max", impl->GetMaxUnsafe());
                    }
                    if (impl->GetVisual() != TFSNumeric::EVisualTypes::Raw) {
                        result.InsertValue("visual", ::ToString(impl->GetVisual()));
                    }
                    if (Traits & EReportTraits::PrecisionField) {
                        result.InsertValue("precision", impl->GetPrecision());
                    }
                }
            }

            {
                auto impl = dynamic_cast<const TFSDuration*>(&element);
                if (impl) {
                    if (!!impl->HasMin()) {
                        result.InsertValue("min", impl->GetMinUnsafe().Seconds());
                    }
                    if (!!impl->HasMax()) {
                        result.InsertValue("max", impl->GetMaxUnsafe().Seconds());
                    }
                }
            }

            return result;
        }

        NJson::TJsonValue TDefaultSerializer::SerializeToJson(const TScheme& scheme) const {
            if (!scheme.GetStructureElement()) {
                return NJson::JSON_NULL;
            } else {
                return SerializeToJsonImpl(*scheme.GetStructureElement());
            }
        }

    }
}
