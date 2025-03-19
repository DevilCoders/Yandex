#include "data_transformation.h"

namespace NCS {
    namespace NObfuscator {
        TDataTransformationObfuscator::TFactory::TRegistrator<TDataTransformationObfuscator> TDataTransformationObfuscator::Registrator(TDataTransformationObfuscator::GetTypeName());

        TString TDataTransformationObfuscator::DoObfuscate(const TStringBuf inputJsonStr) const {
            TFLEventLog::Error("TDataTransformationObfuscator::DoObfuscate(const TStringBuf) unimplemented");
            return TString(inputJsonStr);
        }

        NJson::TJsonValue TDataTransformationObfuscator::DoObfuscate(const NJson::TJsonValue& inputJson) const {
            NJson::TJsonValue outputJson(NJson::JSON_MAP);
            for (auto&& [initialPath, finalPath] : PathMappings) {
                TJsonExtractorByPath extractor(inputJson, outputJson, finalPath);
                if (!extractor.Scan(initialPath)) {
                    TFLEventLog::Error("TDataTransformationObfuscator failed to obfuscate json")("initial_json", inputJson.GetStringRobust());
                    return inputJson;
                }
            }
            return outputJson;
        }

        bool TDataTransformationObfuscator::DoObfuscateInplace(NJson::TJsonValue& json) const {
            auto obfuscatedJson = DoObfuscate(json);
            std::swap(json, obfuscatedJson);
            return true;
        }

        NFrontend::TScheme TDataTransformationObfuscator::DoGetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result = TBase::DoGetScheme(server);
            auto& fields = result.Add<TFSArray>("path_mappings").SetElement<TFSStructure>().SetStructure();
            fields.Add<TFSString>("key", "Путь в исходном json");
            fields.Add<TFSString>("value", "Путь в конечном json");
            return result;
        }

        NJson::TJsonValue TDataTransformationObfuscator::DoSerializeToJson() const {
            NJson::TJsonValue result = TBase::DoSerializeToJson();
            TJsonProcessor::WriteMap(result, "path_mappings", PathMappings);
            return result;
        }

        bool TDataTransformationObfuscator::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TBase::DoDeserializeFromJson(jsonInfo)) {
                return false;
            }
            if (!TJsonProcessor::ReadMap(jsonInfo, "path_mappings", PathMappings)) {
                return false;
            }
            return true;
        }

        TJsonExtractorByPath::TJsonExtractorByPath(const NJson::TJsonValue& initialJson, NJson::TJsonValue& finalJson, const TString& finalPath)
            : TBase(initialJson)
            , FinalJson(finalJson)
            , FinalPath(finalPath)
        {
        }

        bool TJsonExtractorByPath::DoExecute(const NJson::TJsonValue& matchedValue) const {
            const TVector<TString> finalPath = StringSplitter(FinalPath).SplitBySet("[]\n\r").SkipEmpty().ToList<TString>();
            auto* currentNodePtr = &FinalJson;
            for (const auto& nodeName : finalPath) {
                if (!currentNodePtr->Has(nodeName)) {
                    currentNodePtr->InsertValue(nodeName, NJson::JSON_NULL);
                }
                if (!currentNodePtr->GetValuePointer(nodeName, &currentNodePtr)) {
                    return false;
                }
            }
            currentNodePtr->SetValue(matchedValue);
            return true;
        }

    }
}
