#include "white_list.h"

#include <library/cpp/xml/document/xml-document.h>

namespace {
    constexpr i32 MaxDeepLinmit = 32;
}

namespace NCS {

    namespace NObfuscator {
        TWhiteListObfuscator::TFactory::TRegistrator<TWhiteListObfuscator> TWhiteListObfuscator::Registrator(TWhiteListObfuscator::GetTypeName());

        bool TWhiteListObfuscator::ObfuscateJson(NJson::TJsonValue& json, i32 curDeep) const {
            if (curDeep < 0) {
                return false;
            }

            if (json.IsArray()) {
                for (auto& i : json.GetArraySafe()) {
                    if (!ObfuscateJson(i, curDeep - 1)) {
                        i.SetValue(ObfuscatedText);
                    }
                }
            } else if (json.IsMap()) {
                for (auto& [key, value] : json.GetMapSafe()) {
                    if (Keys.contains(key) || ObfuscateJson(value, curDeep - 1)) {
                        continue;
                    } else {
                        value.SetValue(ObfuscatedText);
                    }
                }
            }
            return true;
        }

        bool TWhiteListObfuscator::ObfuscateXml(NXml::TNode& node, i32 curDeep) const {
            if (curDeep < 0) {
                return false;
            }

            for (auto child = node.FirstChild(); !child.IsNull(); child = child.NextSibling()) {
                if (Keys.contains(child.Name()) || ObfuscateXml(child, curDeep - 1)) {
                    continue;
                } else {
                    child.SetValue(ObfuscatedText);
                }
            }
            return true;
        }

        TString TWhiteListObfuscator::ObfuscateXml(const TStringBuf strBuf) const {
            const TString str = TString(strBuf);
            const auto pos = str.find("<?xml");
            if (pos == TString::npos) {
                TFLEventLog::Signal("obfuscator")("&error_type", "parse")("&content_type", "xml");
                return ObfuscatedText;
            }
            const TString xmlStr = str.substr(pos);
            try {
                NXml::TDocument doc(xmlStr, NXml::TDocument::String);
                auto root = doc.Root();
                if (!ObfuscateXml(root, MaxDeep)) {
                    TFLEventLog::Signal("obfuscator")("&error_type", "obfuscate")("&content_type", "xml");
                    return ObfuscatedText;
                }
                return doc.ToString("utf-8");
            } catch (...) {
                TFLEventLog::Signal("obfuscator")("&error_type", "exception")("exception_message", CurrentExceptionMessage())("&content_type", "xml");
                return ObfuscatedText;
            }
        }

        TString TWhiteListObfuscator::ObfuscateJson(const TStringBuf str) const {
            const auto pos = str.find("{");
            if (pos == TString::npos) {
                TFLEventLog::Signal("obfuscator")("&error_type", "parse")("&content_type", "json");
                return ObfuscatedText;
            }
            const TStringBuf jsonStr = str.substr(pos);
            NJson::TJsonValue info;
            if (!NJson::ReadJsonFastTree(jsonStr, &info)) {
                TFLEventLog::Signal("obfuscator")("&error_type", "parse")("&content_type", "json");
                return ObfuscatedText;
            }
            if (!ObfuscateJson(info, MaxDeep)) {
                TFLEventLog::Signal("obfuscator")("&error_type", "obfuscate")("&content_type", "json");
                return ObfuscatedText;
            }
            return info.GetStringRobust();
        }

        NFrontend::TScheme TWhiteListObfuscator::DoGetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result = TBase::DoGetScheme(server);
            result.Add<TFSArray>("keys").SetElement<TFSString>();
            result.Add<TFSNumeric>("max_deep").SetRequired(false);
            result.Add<TFSString>("obfuscated_text").SetRequired(false);
            return result;
        }

        NJson::TJsonValue TWhiteListObfuscator::DoSerializeToJson() const {
            NJson::TJsonValue result = TBase::DoSerializeToJson();
            TJsonProcessor::WriteContainerArray(result, "keys", Keys);
            JWRITE(result, "max_deep", MaxDeep);
            if (MaxDeep > MaxDeepLinmit) {
                return false;
            }
            JWRITE(result, "obfuscated_text", ObfuscatedText);
            return result;
        }

        bool TWhiteListObfuscator::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TBase::DoDeserializeFromJson(jsonInfo)) {
                return false;
            }
            JREAD_STRING_OPT(jsonInfo, "obfuscated_text", ObfuscatedText);
            JREAD_INT_OPT(jsonInfo, "max_deep", MaxDeep);
            return TJsonProcessor::ReadContainer(jsonInfo, "keys", Keys);
        }
    }
}
