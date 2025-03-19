#include "opeanapi.h"
#include <kernel/common_server/library/scheme/fields.h>

namespace NCS {
    namespace NScheme {

        TString TOpenApiSerializer::StoreAsRef(NJson::TJsonValue&& jsonInfo, const TString& firstName) const {
            auto it = ComponentSchemas.find(firstName);
            if (it != ComponentSchemas.end()) {
                if (it->second.GetStringRobust() == jsonInfo.GetStringRobust()) {
                    return it->first;
                }
            } else {
                ComponentSchemas.emplace(firstName, std::move(jsonInfo));
                return firstName;
            }
            for (auto&& i : ComponentSchemas) {
                if (i.second.GetStringRobust() == jsonInfo.GetStringRobust()) {
                    return i.first;
                }
            }
            ComponentSchemas.emplace(firstName + "-" + ::ToString(++ConflictsCounter), std::move(jsonInfo));
            return firstName + "-" + ::ToString(ConflictsCounter);
        }

        void TOpenApiSerializer::SerializeWideToJson(const TFSWideVariants& element, NJson::TJsonValue& result) const {
            auto impl = dynamic_cast<const TFSWideVariants*>(&element);
            CHECK_WITH_LOG(impl);
            NJson::TJsonValue& oneOfJson = result.InsertValue("oneOf", NJson::JSON_ARRAY);
            NJson::TJsonValue& discrImpl = result.InsertValue("discriminator", NJson::JSON_MAP);
            discrImpl.InsertValue("propertyName", impl->GetFieldName());
            NJson::TJsonValue& mappingJson = discrImpl.InsertValue("mapping", NJson::JSON_ARRAY);
            for (auto&& v : impl->GetVariants()) {
                if (!v.HasAdditionalScheme()) {
                    continue;
                }
                TString refId;
                {
                    NJson::TJsonValue objectScheme = NJson::JSON_MAP;
                    objectScheme.InsertValue("type", "object");
                    NJson::TJsonValue& schemeImpl = objectScheme.InsertValue("properties", NJson::JSON_MAP);
                    if (impl->HasCustomStructureId() && !!impl->GetCustomStructureIdUnsafe()) {
                        auto& internalJson = schemeImpl.InsertValue(impl->GetCustomStructureIdUnsafe(), NJson::JSON_MAP);
                        SerializeToJsonImpl(v.GetAdditionalSchemeUnsafe(), internalJson);
                    } else {
                        SerializeToJsonImpl(v.GetAdditionalSchemeUnsafe(), objectScheme);
                    }
                    NJson::TJsonValue& schemeName = schemeImpl.InsertValue(impl->GetFieldName(), NJson::JSON_MAP);
                    //                        schemeName.InsertValue("required", "true");
                    schemeName.InsertValue("type", "string");

                    refId = StoreAsRef(std::move(objectScheme), v.GetRealValue());
                }
                mappingJson.InsertValue(v.GetRealValue(), "#/components/schemas/" + refId);
                auto& oneOfJsonObject = oneOfJson.AppendValue(NJson::JSON_MAP);
                //                    oneOfJsonObject.InsertValue("type", "object");
                oneOfJsonObject.InsertValue("$ref", "#/components/schemas/" + refId);
            }
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const IElement& element) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::WriteDef(result, "description", element.GetDescription(), Default<TString>());
            TJsonProcessor::WriteDef(result, "title", element.GetDescription(), Default<TString>());

            if (!element.GetDefaultValueView().IsNull()) {
                result.InsertValue("default", element.GetDefaultValueView());
            }

            if (element.IsReadOnly()) {
                result.InsertValue("readOnly", true);
            }

            {
                auto impl = dynamic_cast<const TFSBoolean*>(&element);
                if (impl) {
                    result.InsertValue("type", "boolean");
                    return result;
                }
            }

            {
                auto impl = dynamic_cast<const TFSNumeric*>(&element);
                if (impl) {
                    result.InsertValue("type", "number");
                    return result;
                }
            }

            {
                auto impl = dynamic_cast<const TFSString*>(&element);
                if (impl) {
                    result.InsertValue("type", "string");
                    return result;
                }
            }

            {
                auto impl = dynamic_cast<const TFSArray*>(&element);
                if (impl) {
                    result.InsertValue("type", "array");
                    if (impl->GetArrayTypeElement()) {
                        if (!!impl->HasMinItems()) {
                            result.InsertValue("minItems", impl->GetMinItemsUnsafe());
                        }
                        if (!!impl->HasMaxItems()) {
                            result.InsertValue("maxItems", impl->GetMaxItemsUnsafe());
                        }
                        result.InsertValue("items", SerializeToJson(*(impl->GetArrayTypeElement())));
                    }
                }
            }

            {
                auto impl = dynamic_cast<const TFSMap*>(&element);
                if (impl) {
                    result.InsertValue("type", "object");
                    result["additionalProperties"] = SerializeToJson(*impl->GetElementType());
                }
            }

            {
                auto impl = dynamic_cast<const TFSStructure*>(&element);
                if (impl) {
                    result.InsertValue("type", "object");
                    TJsonProcessor::Write(result, "additionalProperties", true);
                    if (impl->GetStructureElement()) {
                        SerializeToJsonImpl(*impl->GetStructureElement(), result);
                    }
                }
            }

            {
                auto impl = dynamic_cast<const TFSVariants*>(&element);
                if (impl) {
                    if (impl->GetVariants().size()) {
                        NJson::TJsonValue variantsJson = NJson::JSON_ARRAY;
                        for (auto&& v : impl->GetVariants()) {
                            variantsJson.AppendValue(v);
                        }
                        if (impl->GetMultiSelect()) {
                            result.InsertValue("type", "array");
                            NJson::TJsonValue itemJson;
                            itemJson.InsertValue("type", "string");
                            itemJson.InsertValue("enum", variantsJson);
                            result.InsertValue("items", itemJson);
                        } else {
                            result.InsertValue("type", "string");
                            result.InsertValue("enum", variantsJson);
                        }
                    } else {
                        result.InsertValue("type", "string");
                    }

                }
            }

            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const TScheme& scheme) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            if (scheme.GetStructureElement()) {
                SerializeToJsonImpl(*scheme.GetStructureElement(), result.InsertValue("schema", NJson::JSON_MAP));
            } else {
                return NJson::JSON_NULL;
            }
            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const TVector<TScheme>& scheme) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            NJson::TJsonValue& oneOfJson = result.InsertValue("schema", NJson::JSON_MAP).InsertValue("oneOf", NJson::JSON_ARRAY);
            for (auto&& i : scheme) {
                if (i.GetStructureElement()) {
                    SerializeToJsonImpl(*i.GetStructureElement(), oneOfJson.AppendValue(NJson::JSON_MAP));
                } else {
                    continue;
                }
            }
            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const THandlerRequestBody& scheme) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            result.InsertValue("required", scheme.IsRequired());
            TJsonProcessor::WriteDef(result, "description", scheme.GetDescription(), Default<TString>());
            NJson::TJsonValue& jsonContent = result.InsertValue("content", NJson::JSON_MAP);
            for (auto&& [contentType, contentInfo] : scheme.GetContentByType()) {
                if (contentInfo.IsEmpty()) {
                    jsonContent.InsertValue(contentType, NJson::JSON_MAP).InsertValue("schema", NJson::JSON_MAP).InsertValue("type", "string");
                } else {
                    jsonContent.InsertValue(contentType, SerializeToJson(contentInfo));
                }
            }
            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const THandlerResponse& scheme) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::WriteDef(result, "description", scheme.GetDescription(), Default<TString>());
            NJson::TJsonValue& jsonHeaders = result.InsertValue("headers", NJson::JSON_MAP);
            for (auto&& [_, h] : scheme.GetHeaders()) {
                auto& jsonHeader = jsonHeaders.InsertValue(h->GetFieldName(), NJson::JSON_MAP);
                jsonHeader.InsertValue("description", h->GetDescription());
                jsonHeader.InsertValue("schema", SerializeToJson(*h));
            }

            NJson::TJsonValue& jsonContent = result.InsertValue("content", NJson::JSON_MAP);
            for (auto&& [contentType, contentInfo] : scheme.GetContentByType()) {
                jsonContent.InsertValue(contentType, SerializeToJson(contentInfo));
            }
            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const THandlerRequestParameters& scheme) const {
            NJson::TJsonValue result = NJson::JSON_ARRAY;
            for (auto&& i : scheme.GetParameters()) {
                NJson::TJsonValue jsonElement = NJson::JSON_MAP;
                jsonElement.InsertValue("name", i->GetFieldName());
                jsonElement.InsertValue("in", i->GetOriginator());
                jsonElement.InsertValue("required", i->IsRequired());
                jsonElement.InsertValue("schema", SerializeToJson(*i));
                result.AppendValue(std::move(jsonElement));
            }
            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const THandlerRequestMethod& scheme) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::WriteDef(result, "description", scheme.GetDescription(), Default<TString>());
            TJsonProcessor::WriteDef(result, "summary", scheme.GetSummary(), Default<TString>());
            TJsonProcessor::WriteDef(result, "operationId", scheme.GetOperationId(), Default<TString>());
            NJson::TJsonValue& jsonResponses = result.InsertValue("responses", NJson::JSON_MAP);
            for (auto&& [responseCode, response] : scheme.GetResponses()) {
                jsonResponses.InsertValue(responseCode, SerializeToJson(response));
            }
            if (scheme.HasRequestBody()) {
                result.InsertValue("requestBody", SerializeToJson(scheme.GetRequestBodyUnsafe()));
            }
            result.InsertValue("parameters", SerializeToJson(scheme.GetParameters()));
            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const THandlerSchemasCollection& schemasCollection) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            NJson::TJsonValue& pathsJson = result.InsertValue("paths", NJson::JSON_MAP);
            for (auto&& scheme : schemasCollection.GetSchemas()) {
                TString path;
                for (auto&& i : scheme.GetPath()) {
                    if (i.GetValue()) {
                        path += "/" + i.GetValue();
                    } else if (i.GetVariable()) {
                        path += "/{" + i.GetVariable() + "}";
                    }
                }
                if (!path) {
                    path = "/";
                }
                pathsJson.InsertValue(path, SerializeToJson(scheme));
            }
            NJson::TJsonValue& commonSchemasJson = result.InsertValue("components", NJson::JSON_MAP).InsertValue("schemas", NJson::JSON_MAP);
            for (auto&& i : ComponentSchemas) {
                commonSchemasJson.InsertValue(i.first, std::move(i.second));
            }
            return result;
        }

        NJson::TJsonValue TOpenApiSerializer::SerializeToJson(const THandlerScheme& scheme) const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            for (auto&& [method, methodScheme] : scheme.GetRequestMethods()) {
                result.InsertValue(::ToString(method), SerializeToJson(methodScheme));
            }
            result.InsertValue("parameters", SerializeToJson(scheme.GetParameters()));
            return result;
        }

        void TOpenApiSerializer::SerializeToJsonImpl(const TStructureImpl& scheme, NJson::TJsonValue& result) const {
            if (scheme.GetElements().size() == 1 && scheme.GetElements().front()->GetFieldName() == "*") {
                result.InsertValue("additionalProperties", true);
                return;
            }

            for (auto&& i : scheme.GetElements()) {
                if (!dynamic_cast<const TFSWideVariants*>(i.Get())) {
                    continue;
                }
                Y_ASSERT(!!i->GetFieldName());
                if (!!i->GetFieldName()) {
                    SerializeWideToJson(*dynamic_cast<const TFSWideVariants*>(i.Get()), result);
                }
            }

            result.InsertValue("type", "object");
            NJson::TJsonValue& propsJson = result.InsertValue("properties", NJson::JSON_MAP);
            for (auto&& i : scheme.GetElements()) {
                if (dynamic_cast<const TFSWideVariants*>(i.Get())) {
                    continue;
                }
                Y_ASSERT(!!i->GetFieldName());
                if (!!i->GetFieldName()) {
                    propsJson.InsertValue(i->GetFieldName(), SerializeToJson(*i));
                }
            }
        }

    }
}
