#include "obfuscate_operator.h"

#include <kernel/common_server/secret/abstract/manager.h>

namespace NCS {
    namespace NObfuscator {
        using EPolicy = IObfuscateOperator::EPolicy;

        NJson::TJsonValue TObfuscateWithText::SerializeToJson() const {
            NJson::TJsonValue jsonInfo;
            JWRITE(jsonInfo, "obfuscated_text", ObfuscatedText);
            return jsonInfo;
        }
        bool TObfuscateWithText::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            JREAD_STRING_OPT(jsonInfo, "obfuscated_text", ObfuscatedText);
            return true;
        }
        NFrontend::TScheme TObfuscateWithText::GetScheme(const IBaseServer& /*server*/) const {
            NFrontend::TScheme result;
            result.Add<TFSString>("obfuscated_text", "На какой текст заменять хэдер").SetRequired(false);
            return result;
        }
        TString TObfuscateWithText::Obfuscate(const TStringBuf /*str*/) const {
            return ObfuscatedText;
        }

        NJson::TJsonValue TObfuscateWithHash::SerializeToJson() const {
            return NJson::JSON_MAP;
        }
        bool TObfuscateWithHash::DeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) {
            return true;
        }
        NFrontend::TScheme TObfuscateWithHash::GetScheme(const IBaseServer& /*server*/) const {
            return NFrontend::TScheme();
        }
        TString TObfuscateWithHash::Obfuscate(const TStringBuf str) const {
            return ToString(FnvHash<ui64>(str));
        }

        NJson::TJsonValue TNoObfuscate::SerializeToJson() const {
            return NJson::JSON_MAP;
        }
        bool TNoObfuscate::DeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) {
            return true;
        }
        NFrontend::TScheme TNoObfuscate::GetScheme(const IBaseServer& /*server*/) const {
            return NFrontend::TScheme();
        }
        TString TNoObfuscate::Obfuscate(const TStringBuf str) const {
            return ToString(str);
        }

        NJson::TJsonValue TObfuscateWithCipher::SerializeToJson() const {
            NJson::TJsonValue jsonInfo;
            JWRITE(jsonInfo, "cipher", CipherName);
            return jsonInfo;
        }
        bool TObfuscateWithCipher::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            JREAD_STRING(jsonInfo, "cipher", CipherName);
            return true;
        }
        NFrontend::TScheme TObfuscateWithCipher::GetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result;
            result.Add<TFSVariants>("cipher").SetVariants(server.GetCipherNames()).SetRequired(true);
            return result;
        }
        TString TObfuscateWithCipher::Obfuscate(const TStringBuf str) const {
            TString res;
            auto cipher = ICSOperator::GetServer().GetCipherPtr(CipherName);
            if (!cipher) {
                TFLEventLog::Signal("obfuscator")("&error_type", "null_cipher");
                return "Error obfuscate";
            }
            if (!cipher->Encrypt(TString(str), res)) {
                TFLEventLog::Signal("obfuscator")("&error_type", "cipher_encrypt")("&cipher_name", cipher->GetName());
                return "Error obfuscate";
            }
            return res;
        }

        NJson::TJsonValue TObfuscateWithTokenizator::SerializeToJson() const {
            NJson::TJsonValue jsonInfo;
            JWRITE(jsonInfo, "tokenizator", TokenizitorName);
            return jsonInfo;
        }
        bool TObfuscateWithTokenizator::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            JREAD_STRING(jsonInfo, "tokenizator", TokenizitorName);
            return true;
        }
        NFrontend::TScheme TObfuscateWithTokenizator::GetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result;
            result.Add<TFSVariants>("tokenizator").SetVariants(server.GetSecretManagerNames()).SetRequired(true);
            return result;
        }
        TString TObfuscateWithTokenizator::Obfuscate(const TStringBuf str) const {
            const TString res = TGUID::CreateTimebased().AsGuidString();
            auto tokenizator = ICSOperator::GetServer().GetSecretManager(TokenizitorName);
            if (!tokenizator) {
                TFLEventLog::Signal("obfuscator")("&error_type", "null_tokenizator");
                return "Error obfuscate";
            }
            TVector<NSecret::TDataToken> tokens;
            tokens.emplace_back(NSecret::TDataToken::Build(res, TString(str)));
            if (!tokenizator->Encode(tokens, true)) {
                TFLEventLog::Signal("obfuscator")("&error_type", "bad tokenizator");
                return "Error obfuscate";
            }
            return res;
        }

        TObfuscateWithText::TFactory::TRegistrator<TObfuscateWithText> TObfuscateWithText::Registrator(TObfuscateWithText::GetTypeName());
        TObfuscateWithHash::TFactory::TRegistrator<TObfuscateWithHash> TObfuscateWithHash::Registrator(TObfuscateWithHash::GetTypeName());
        TObfuscateWithCipher::TFactory::TRegistrator<TObfuscateWithCipher> TObfuscateWithCipher::Registrator(TObfuscateWithCipher::GetTypeName());
        TNoObfuscate::TFactory::TRegistrator<TNoObfuscate> TNoObfuscate::Registrator(TNoObfuscate::GetTypeName());
        TObfuscateWithTokenizator::TFactory::TRegistrator<TObfuscateWithTokenizator> TObfuscateWithTokenizator::Registrator(TObfuscateWithTokenizator::GetTypeName());
    }
}
