#pragma once
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/api/history/common.h>

class IBaseServer;

namespace NCS {

    namespace NObfuscator {

        class TObfuscatorKey;

        class IObfuscator: public IJsonDecoderSerializable {
        public:
            enum class EContentType {
                Json,
                Xml,
                Text,
                Header,
            };

        private:
            CS_ACCESS(IObfuscator, EContentType, ContentType, EContentType::Json);
            CSA_FLAG(IObfuscator, Testing, false);

        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const = 0;
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual NJson::TJsonValue DoSerializeToJson() const = 0;
            virtual TString DoObfuscate(const TStringBuf str) const = 0;

            virtual NJson::TJsonValue DoObfuscate(const NJson::TJsonValue& inputJson) const {
                return inputJson;
            }

            virtual bool DoObfuscateInplace(NJson::TJsonValue& /*json*/) const {
                return false;
            }

        public:
            using TFactory = NObjectFactory::TObjectFactory<IObfuscator, TString>;
            using TPtr = TAtomicSharedPtr<IObfuscator>;

            class TObfuscatorMessage: public NMessenger::IMessage {
            public:
                TObfuscatorMessage() = default;

            private:
                CSA_DEFAULT(TObfuscatorMessage, TString, InitialText);
                CSA_DEFAULT(TObfuscatorMessage, TString, ObfuscatedText);
            };

            static EContentType ToObfuscatorContentType(const TString& contentType) {
                if (contentType == "application/json") {
                    return EContentType::Json;
                } else if (contentType.find("xml") != TString::npos) {
                    return EContentType::Xml;
                } else if (contentType == "header") {
                    return EContentType::Header;
                }
                return EContentType::Text;
            }

            TString Obfuscate(const TStringBuf str) const {
                const TString obfuscatedText = DoObfuscate(str);
                if (IsTesting()) {
                    TObfuscatorMessage message;
                    message.SetInitialText(TString(str));
                    message.SetObfuscatedText(obfuscatedText);
                    SendGlobalMessage<TObfuscatorMessage>(message);
                }
                return obfuscatedText;
            }

            TString Obfuscate(const TString& str) const {
                return Obfuscate(TStringBuf(str));
            }

            NJson::TJsonValue Obfuscate(const NJson::TJsonValue& inputJson) const {
                const NJson::TJsonValue outputJson = DoObfuscate(inputJson);
                if (IsTesting()) {
                    TObfuscatorMessage message;
                    message.SetInitialText(inputJson.GetStringRobust());
                    message.SetObfuscatedText(outputJson.GetStringRobust());
                    SendGlobalMessage<TObfuscatorMessage>(message);
                }
                return outputJson;
            }

            bool ObfuscateInplace(NJson::TJsonValue& json) const {
                if (!DoObfuscateInplace(json)) {
                    return false;
                }
                if (IsTesting()) {
                    TObfuscatorMessage message;
                    message.SetObfuscatedText(json.GetStringRobust());
                    SendGlobalMessage<TObfuscatorMessage>(message);
                }
                return true;
            }

            virtual ~IObfuscator() = default;
            virtual TString GetClassName() const = 0;
            virtual size_t GetRulesCount() const = 0;
            virtual bool IsMatch(const TObfuscatorKey& obfuscatorKey) const = 0;

            virtual NJson::TJsonValue SerializeToJson() const final {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::Write(result, "testing", TestingFlag);
                JWRITE_ENUM(result, "content_type", ContentType);
                TJsonProcessor::Write(result, "params", DoSerializeToJson());
                return result;
            }
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!DoDeserializeFromJson(jsonInfo["params"])) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "testing", TestingFlag)) {
                    return false;
                }
                JREAD_FROM_STRING(jsonInfo, "content_type", ContentType);
                return true;
            }

            NFrontend::TScheme GetScheme(const IBaseServer& server) const {
                NFrontend::TScheme result;
                result.Add<TFSVariants>("content_type").InitVariants<EContentType>();
                result.Add<TFSStructure>("params").SetStructure(DoGetScheme(server));
                return result;
            }
        };

        class TObfuscatorContainerConfiguration: public TDefaultInterfaceContainerConfiguration {
        public:
            static TString GetSpecialSectionForType(const TString& /*className*/) {
                return "data";
            }
        };

        class TObfuscatorContainer: public TInterfaceContainer<IObfuscator, TObfuscatorContainerConfiguration> {
        private:
            using TBase = TInterfaceContainer<IObfuscator, TObfuscatorContainerConfiguration>;

        public:
            TString Obfuscate(const TStringBuf str) const {
                return !!Object ? Object->Obfuscate(str) : TString(str);
            }

            template <class T>
            T Obfuscate(const T& input) const {
                return !!Object ? Object->Obfuscate(input) : input;
            }

            template <class T>
            bool ObfuscateInplace(T& input) const {
                return !!Object ? Object->ObfuscateInplace(input) : true;
            }

            using TBase::TBase;
        };

        class TObfuscatorKey {
        private:
            using EContentType = IObfuscator::EContentType;
            CS_ACCESS(TObfuscatorKey, EContentType, Type, EContentType::Json);

        public:
            virtual ~TObfuscatorKey() = default;
            explicit TObfuscatorKey(const EContentType type = EContentType::Json)
                : Type(type)
            {
            }

            explicit TObfuscatorKey(const TString& type)
                : Type(IObfuscator::ToObfuscatorContentType(type))
            {
            }

            template <class T>
            const T* GetAs() const {
                return dynamic_cast<const T*>(this);
            }
        };

        class TObfuscatorKeyMap: public TObfuscatorKey {
        private:
            using TBase = TObfuscatorKey;
            using EContentType = IObfuscator::EContentType;
            using TParamsMap = TMap<TCiString, TCiString>;
            CSA_DEFAULT(TObfuscatorKeyMap, TParamsMap, Params);

        public:
            TObfuscatorKeyMap() = default;
            explicit TObfuscatorKeyMap(const TCiString& type)
                : TBase(type)
            {
            }
            explicit TObfuscatorKeyMap(TMap<TCiString, TCiString>&& params)
                : TObfuscatorKeyMap(EContentType::Json, std::move(params))
            {
            }

            TObfuscatorKeyMap(const EContentType type, TMap<TCiString, TCiString>&& params)
                : TBase(type)
                , Params(std::move(params))
            {
            }

            void Add(const TCiString& key, const TCiString& value) {
                Params.emplace(key, value);
            }
        };

        class IObfuscatorManager {
        public:
            using TPtr = TAtomicSharedPtr<IObfuscatorManager>;
            virtual ~IObfuscatorManager() = default;

            virtual IObfuscator::TPtr GetObfuscatorFor(const TObfuscatorKey& key) const = 0;

            TString Obfuscate(const TObfuscatorKey& key, const TStringBuf str) const {
                const auto obfuscator = GetObfuscatorFor(key);
                return !!obfuscator ? obfuscator->Obfuscate(str) : TString(str);
            }

            template <class T>
            T Obfuscate(const TObfuscatorKey& key, const T& input) const {
                const auto obfuscator = GetObfuscatorFor(key);
                return !!obfuscator ? obfuscator->Obfuscate(input) : input;
            }

            template <class T>
            bool ObfuscateInplace(const TObfuscatorKey& key, T& input) const {
                const auto obfuscator = GetObfuscatorFor(key);
                return !!obfuscator ? obfuscator->ObfuscateInplace(input) : true;
            }

            template <class T>
            const T* GetAs() const {
                return dynamic_cast<const T*>(this);
            }
            template <class T>
            T* GetAs() {
                return dynamic_cast<T*>(this);
            }

            virtual bool Start() noexcept = 0;
            virtual bool Stop() noexcept = 0;
        };

        class TObfuscatorManagerContainer: public TBaseInterfaceContainer<IObfuscatorManager> {
        private:
            using TBase = TBaseInterfaceContainer<IObfuscatorManager>;
        public:
            TObfuscatorContainer GetObfuscatorFor(const TObfuscatorKey& key) const {
                return !!Object ? Object->GetObfuscatorFor(key) : nullptr;
            }
            TString Obfuscate(const TObfuscatorKey& key, const TStringBuf str) const {
                return !!Object ? Object->Obfuscate(key, str) : TString(str);
            }

            template <class T>
            T Obfuscate(const TObfuscatorKey& key, const T& input) const {
                return !!Object ? Object->Obfuscate(key, input) : input;
            }

            template <class T>
            bool ObfuscateInplace(const TObfuscatorKey& key, T& input) const {
                return !!Object ? Object->ObfuscateInplace(key, input) : true;
            }
            using TBase::TBase;
        };

        class IObfuscatorWithRules: public IObfuscator {
        private:
            using TMatchRules = TMap<TCiString, TSet<TCiString>>;
            CSA_DEFAULT(IObfuscatorWithRules, TMatchRules, MatchRules);

        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const override;

        public:
            virtual NJson::TJsonValue DoSerializeToJson() const override;
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
            virtual bool IsMatch(const TObfuscatorKey& obfuscatorKey) const final;
            virtual size_t GetRulesCount() const final {
                return MatchRules.size();
            }

            void AddRules(const TCiString& key, const TSet<TCiString>& rules) {
                MatchRules.emplace(key, rules);
            }
        };
    }
}
