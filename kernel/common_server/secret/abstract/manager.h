#pragma once
#include <kernel/common_server/roles/abstract/manager.h>
#include <kernel/common_server/api/links/manager.h>
#include <kernel/common_server/common/manager_config.h>
#include <library/cpp/digest/md5/md5.h>

namespace NCS {
    namespace NSecret {
        class ISecretsManager;

        class TDataToken {
        private:
            CSA_PROTECTED_DEF(TDataToken, TString, SecretId);
            CSA_PROTECTED_DEF(TDataToken, TString, Data);
            CSA_PROTECTED_DEF(TDataToken, TString, DataHash);
            CSA_PROTECTED_DEF(TDataToken, TString, DataType);
            CSA_PROTECTED(TDataToken, TInstant, Timestamp, TInstant::Now());
            CSA_PROTECTED_DEF(TDataToken, TString, UserId);
            CSA_PROTECTED_DEF(TDataToken, TString, Scope);
        public:
            NJson::TJsonValue SerializeToJson() const;
            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
            static NFrontend::TScheme GetScheme(const IBaseServer& server);

            TDataToken() = default;
            TDataToken(const TDataToken& base) = default;
            static TDataToken BuildFromSecretId(const TString& secretId) {
                TDataToken result;
                result.SecretId = secretId;
                return result;
            }
            static TDataToken BuildFromData(const TString& data) {
                TDataToken result;
                result.Data = data;
                result.DataHash = MD5::Calc(data);
                return result;
            }
            static TDataToken Build(const TString& secretId, const TString& data) {
                TDataToken result;
                result.SecretId = secretId;
                result.Data = data;
                result.DataHash = MD5::Calc(data);
                return result;
            }
        };

        class ISecretsManager {
        protected:
            virtual bool DoEncode(TVector<TDataToken>& data, const bool writeIfNotExists) const = 0;
            virtual bool DoDecode(TVector<TDataToken>& data, const bool forceCheckExistance) const = 0;
        public:
            using TPtr = TAtomicSharedPtr<ISecretsManager>;
            virtual ~ISecretsManager() = default;
            virtual bool SecretsStart() {
                return true;
            }
            virtual bool SecretsStop() {
                return true;
            }
            bool Encode(TVector<TDataToken>& data, const bool writeIfNotExists) const {
                for (auto&& i : data) {
                    if (!i.GetData()) {
                        TFLEventLog::Error("incorrect data for encoding - empty");
                        return false;
                    }
                }
                return DoEncode(data, writeIfNotExists);
            }
            bool Decode(TVector<TDataToken>& data, const bool forceCheckExistance) const {
                for (auto&& i : data) {
                    if (!i.GetSecretId()) {
                        TFLEventLog::Error("incorrect decoding data - empty secret id");
                        return false;
                    }
                    if (!!i.GetData()) {
                        TFLEventLog::Warning("Filled data would be ignored");
                    }
                }
                return DoDecode(data, forceCheckExistance);
            }
        };

        using IManager = ISecretsManager;
    }
}
