#pragma once
#include "object.h"
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/mediator/messenger.h>
#include <kernel/common_server/common/abstract.h>
#include <kernel/common_server/util/accessor.h>
#include <util/datetime/base.h>
#include <util/string/vector.h>
#include <kernel/common_server/api/history/event.h>

namespace NCS {
    class ISettingsConfig;

    class IReadSettings {
    public:
        virtual ~IReadSettings() = default;
        virtual bool GetValueStr(const TString& key, TString& result) const = 0;
        template <class T>
        T GetHandlerValueDef(const TString& handlerName, const TString& key, const T defValue) const {
            return GetValueDef("handlers." + handlerName + "." + key, defValue);
        }

        template <class T>
        T GetBackgroundValueDef(const TString& processId, const TString& key, const T defValue) const {
            return GetValueDef("backgrounds." + processId + "." + key, defValue);
        }

        template <class T>
        T GetValueDef(const TString& key, const T defValue) const {
            T result;
            if (GetValue<T>(key, result)) {
                return result;
            }
            return defValue;
        }

        template <class T>
        T GetValueDef(const TVector<TString>& pathes, const TString& key, const T defValue, IUserProcessorCustomizer::TPtr customizer = nullptr) const {
            const TString suffix = (key ? ("." + key) : "");
            for (auto&& i : pathes) {

                TString resultStr;
                if (customizer && customizer->OverrideOption(i + suffix, resultStr)) {
                    T result;
                    if (TryFromString(resultStr, result)) {
                        return result;
                    } else {
                        WARNING_LOG << "Cannot parse value from '" << resultStr << "' string (" << key << ")" << Endl;
                        return defValue;
                    }
                }

                T result;
                if (GetValue(i + suffix, result)) {
                    return result;
                }
            }
            return defValue;
        }

        template <class T>
        bool GetValue(const TString& key, T& result) const {
            TString resultStr;
            if (!GetValueStr(key, resultStr)) {
                return false;
            }
            return TryFromString(resultStr, result);
        }

        template <>
        bool GetValue<NJson::TJsonValue>(const TString& key, NJson::TJsonValue& result) const {
            TString resultStr;
            if (!GetValueStr(key, resultStr)) {
                return false;
            }
            NJson::TJsonValue resultLocal;
            if (NJson::ReadJsonFastTree(resultStr, &resultLocal)) {
                std::swap(resultLocal, result);
                return true;
            } else {
                result = resultStr;
                return true;
            }
        }

        bool GetStrings(const TString& key, TSet<TString>& result) const {
            TString resultStr;
            if (!GetValueStr(key, resultStr)) {
                return false;
            }
            TVector<TString> valuesVec = SplitString(resultStr, ",");
            result.insert(valuesVec.begin(), valuesVec.end());
            return true;
        }

        template <class T>
        TMaybe<T> GetValue(const TString& key) const {
            T result;
            if (!GetValue(key, result)) {
                return {};
            } else {
                return result;
            }
        }

    };

    class ISettings: public IMessageProcessor, public IReadSettings {
    public:
        using TPtr = TAtomicSharedPtr<ISettings>;
        class TMessageGetSettingValue: public IMessage {
            CSA_READONLY_DEF(TString, SettingName);
            CSA_MAYBE(TMessageGetSettingValue, TString, Value);
        public:
            TMessageGetSettingValue(const TString& name)
                : SettingName(name)
            {
            }

            template <class T>
            T GetValueDef(const T defValue) const {
                if (HasValue()) {
                    ui32 value;
                    if (TryFromString(*Value, value)) {
                        return value;
                    }
                }
                return defValue;
            }
        };

    public:
        ISettings(const ISettingsConfig& config);
        virtual ~ISettings();

        virtual bool Process(IMessage* message) override;

        virtual TString Name() const override {
            return ToString((ui64)this);
        }

        virtual bool HasValues(const TSet<TString>& keys, TSet<TString>& existKeys) const = 0;
        virtual bool RemoveKeys(const TVector<TString>& keys, const TString& userId) const = 0;
        virtual bool SetValues(const TVector<TSetting>& values, const TString& userId) const = 0;
        virtual bool GetHistory(const TInstant since, TVector<TAtomicSharedPtr<TObjectEvent<TSetting>>>& result) const = 0;
        virtual bool GetAllSettings(TVector<TSetting>& result) const = 0;

        bool SetValue(const TString& key, const TString& value, const TString& userId) const {
            TSetting s(key, value);
            return SetValues({s}, userId);
        }

    };

}

namespace NFrontend {
    using IReadSettings = NCS::IReadSettings;
    using ISettings = NCS::ISettings;
    namespace NSettings {
        template <class T>
        static T GetValueDef(const TString& key, const T defValue) {
            ISettings::TMessageGetSettingValue mess(key);
            SendGlobalMessage(mess);
            T result;
            if (mess.HasValue() && TryFromString(mess.GetValueUnsafe(), result)) {
                return result;
            }
            return defValue;
        }
    }
}

using ISettings = NFrontend::ISettings;
