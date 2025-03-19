#pragma once
#include "abstract.h"
#include "scheme.h"
#include <kernel/common_server/util/json_processing.h>
#include <util/generic/serialized_enum.h>

namespace NCS {
    namespace NScheme {
        class TFSIgnore: public IElement {
        private:
            using TBase = IElement;
        public:
            using TBase::TBase;
            virtual EElementType GetType() const override {
                return EElementType::Ignore;
            }
            virtual bool DoValidateJson(const NJson::TJsonValue& /*json*/, const TString& /*path*/, TString& /*error*/) const override {
                return true;
            }

            virtual void MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const override;

        private:
            static IElement::TFactory::TRegistrator<TFSIgnore> Registrator;
        };

        class TFSString: public IDefaultSchemeElement<TString> {
        public:
            enum class EVisualType {
                Color /* "color" */,
                UUID /* "UUID" */,
                GUID /* "GUID" */,
                Unknown /* "unknown" */,
                Duration /* "duration" */,
                Json /* "json" */,
                Text /* "text" */,  // Enables word wrapping in multiline block.
            };

            CS_ACCESS(TFSString, EVisualType, Visual, EVisualType::Unknown);
            CSA_FLAG(TFSString, MultiLine, false);
        private:
            using TBase = IDefaultSchemeElement<TString>;
        public:
            using TBase::TBase;
            virtual EElementType GetType() const override {
                return EElementType::String;
            }

            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& path, TString& error) const override;

            virtual EElementType GetRegisterType() const override {
                return GetType();
            }

            virtual EVisualType GetVisualType() const {
                return Visual;
            }

            static IElement::TFactory::TRegistrator<TFSString> Registrator;
        };

        class TFSBoolean: public IDefaultSchemeElement<bool> {
        private:
            using TBase = IDefaultSchemeElement<bool>;
        public:
            using TBase::TBase;
            virtual EElementType GetType() const override {
                return EElementType::Boolean;
            }
            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& /*path*/, TString& error) const override {
                if (!json.IsBoolean()) {
                    error = "is not a Boolean";
                }
                return json.IsBoolean();
            }

            static IElement::TFactory::TRegistrator<TFSBoolean> Registrator;
        };

        class TFSArray: public IElement {
            RTLINE_ACCEPTOR_DEF(TFSArray, ArrayTypeElement, IElement::TPtr);
            RTLINE_ACCEPTOR_MAYBE(TFSArray, MinItems, ui64);
            RTLINE_ACCEPTOR_MAYBE(TFSArray, MaxItems, ui64);
        private:
            using TBase = IElement;
        public:
            using TBase::TBase;

            template <class TElement, class... TArgs>
            static THolder<TFSArray> Build(TArgs... args) {
                auto result = MakeHolder<TFSArray>();
                TElement element(args...);
                result->SetElement(element);
                return result;
            }

            TFSArray& SetRequired(const bool value) {
                IElement::SetRequired(value);
                return *this;
            }

            template <class T>
            T& SetElement() {
                ArrayTypeElement = MakeAtomicShared<T>();
                return *VerifyDynamicCast<T*>(ArrayTypeElement.Get());
            }

            template <class T>
            T& SetElement(const T& element) {
                ArrayTypeElement = MakeAtomicShared<T>(element);
                return *VerifyDynamicCast<T*>(ArrayTypeElement.Get());
            }

            IElement& SetElement(THolder<IElement>&& element) {
                ArrayTypeElement = element.Release();
                return *ArrayTypeElement;
            }

            template <class T>
            TFSArray& InitElement() {
                SetElement<T>();
                return *this;
            }

            template <class T>
            TFSArray& InitElement(const T& element) {
                SetElement<T>(element);
                return *this;
            }

            TFSArray& InitElement(THolder<IElement>&& element) {
                SetElement(std::move(element));
                return *this;
            }

            virtual EElementType GetType() const override {
                return EElementType::Array;
            }

            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& path, TString& error) const override;

            virtual void MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const override;

        private:
            virtual void AddValueToDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const override;

            static IElement::TFactory::TRegistrator<TFSArray> Registrator;
        };

        class TFSMap: public IElement {
            CSA_DEFAULT(TFSMap, IElement::TPtr, ElementType);
            CSA_DEFAULT(TFSMap, IElement::TPtr, KeyType);
        private:
            using TBase = IElement;
        public:
            using TBase::TBase;

            TFSMap& SetRequired(const bool value) {
                IElement::SetRequired(value);
                return *this;
            }

            template <class T>
            TFSMap& Key() {
                KeyType = MakeAtomicShared<T>();
                return *this;
            }

            template <class T>
            T& Value() {
                ElementType = MakeAtomicShared<T>();
                return *VerifyDynamicCast<T*>(ElementType.Get());
            }

            template <class T>
            T& Value(const T& elementScheme) {
                ElementType = MakeAtomicShared<T>(elementScheme);
                return *VerifyDynamicCast<T*>(ElementType.Get());
            }

            virtual EElementType GetType() const override {
                return EElementType::Map;
            }
        };

        class TFSWideVariants: public IDefaultSchemeElement<TString> {
        private:
            using TBase = IDefaultSchemeElement<TString>;
            static IElement::TFactory::TRegistrator<TFSWideVariants> Registrator;
        public:
            class TVariant {
            private:
                CSA_DEFAULT(TVariant, TString, RealValue);
                CSA_DEFAULT(TVariant, TString, ValueName);
                CSA_DEFAULT(TVariant, TString, ValueDescription);
                CSA_DEFAULT(TVariant, TString, ValueLink);
                CSA_MAYBE(TVariant, TScheme, AdditionalScheme);
            public:
                NJson::TJsonValue SerializeToJson() const {
                    NJson::TJsonValue result = NJson::JSON_MAP;
                    result.InsertValue("real", RealValue);
                    result.InsertValue("name", ValueName);
                    result.InsertValue("description", ValueDescription);
                    result.InsertValue("link", ValueLink);
                    if (AdditionalScheme) {
                        result.InsertValue("additionalScheme", AdditionalScheme->SerializeToJson());
                    }
                    return result;
                }
            };
        private:
            CSA_DEFAULT(TFSWideVariants, TVector<TVariant>, Variants);
            CS_ACCESS(TFSWideVariants, bool, MultiSelect, false);
            CSA_MAYBE(TFSWideVariants, TString, CustomStructureId);
        public:
            template <class Interface, class TServer>
            TFSWideVariants& InitVariants(const TServer& server) {
                TSet<TString> keys;
                Interface::TFactory::GetRegisteredKeys(keys);
                for (auto&& i : keys) {
                    THolder<Interface> h(Interface::TFactory::Construct(i));
                    if (!h) {
                        continue;
                    }
                    TVariant v;
                    v.SetRealValue(i);
                    v.SetAdditionalScheme(h->GetScheme(server));
                    Variants.emplace_back(std::move(v));
                }
                return *this;
            }

            virtual EElementType GetType() const override {
                return EElementType::WideVariants;
            }

            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& /*path*/, TString& error) const override;
            using TBase::TBase;
        };

        class TFSVariants: public IDefaultSchemeElement<TString> {
            RTLINE_READONLY_ACCEPTOR_DEF(Variants, TSet<TString>);
            RTLINE_ACCEPTOR(TFSVariants, Editable, bool, false);
            RTLINE_ACCEPTOR(TFSVariants, MultiSelect, bool, false);
            RTLINE_ACCEPTOR(TFSVariants, NeedTranslate, bool, false);
            RTLINE_ACCEPTOR_DEF(TFSVariants, Visual, TString);
        private:
            using TBase = IDefaultSchemeElement<TString>;
        public:
            using TBase::TBase;

            virtual EElementType GetType() const override {
                if (MultiSelect) {
                    return EElementType::MultiVariants;
                } else {
                    return EElementType::Variants;
                }
            }

            TSet<TString>& MutableVariants() {
                return Variants;
            }

            template <class T>
            TFSVariants& InitVariantsClass() {
                TSet<TString> keys;
                T::TFactory::GetRegisteredKeys(keys);
                Variants = keys;
                return *this;
            }

            template <class TSelectiveClass, class TBaseClass>
            TFSVariants& InitVariantsClassSelective() {
                TSet<TString> keys;
                TBaseClass::TFactory::GetRegisteredKeys(keys);
                TSet<TString> selectiveKeys;
                for (auto&& i : keys) {
                    THolder<TBaseClass> bClass(TBaseClass::TFactory::Construct(i));
                    if (!dynamic_cast<const TSelectiveClass*>(bClass.Get())) {
                        continue;
                    }
                    selectiveKeys.emplace(i);
                }
                Variants = selectiveKeys;
                return *this;
            }

            template <class TEnum>
            TFSVariants& InitVariants() {
                for (auto&& i : GetEnumNames<TEnum>()) {
                    Variants.emplace(i.second);
                }
                return *this;
            }

            template <class TEnum>
            TFSVariants& InitVariantsAsInteger() {
                for (auto&& i : GetEnumAllValues<TEnum>()) {
                    Variants.emplace(::ToString((i64)i));
                }
                return *this;
            }

            template <class T>
            TFSVariants& SetVariants(const std::initializer_list<T>& container) {
                for (auto&& i : container) {
                    Variants.emplace(::ToString(i));
                }
                return *this;
            }

            template <class T>
            TFSVariants& MulVariantsLeft(const std::initializer_list<T>& container) {
                TSet<TString> variantsOld;
                std::swap(Variants, variantsOld);
                for (auto&& v : variantsOld) {
                    for (auto&& i : container) {
                        Variants.emplace(::ToString(i) + ::ToString(v));
                    }
                }
                return *this;
            }

            template <class T>
            TFSVariants& SetVariants(const TMap<TString, T>& container) {
                for (auto&& i : container) {
                    Variants.emplace(::ToString(i.first));
                }
                return *this;
            }

            template <class TContainer>
            TFSVariants& SetVariants(const TContainer& container) {
                Variants.clear();
                return AddVariants(container);
            }

            template <class TContainer>
            TFSVariants& AddVariants(const TContainer& container) {
                for (auto&& i : container) {
                    Variants.emplace(::ToString(i));
                }
                return *this;
            }

            template <class T>
            TFSVariants& AddVariant(const T& v) {
                Variants.emplace(::ToString(v));
                return *this;
            }

            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& path, TString& error) const override;

            virtual TString AdditionalDescription() const override;

        private:
            static IElement::TFactory::TRegistrator<TFSVariants> Registrator;
        };

        class TFSDuration: public TFSString {
        private:
            using TBase = TFSString;
            RTLINE_ACCEPTOR_MAYBE(TFSDuration, Max, TDuration);
            RTLINE_ACCEPTOR_MAYBE(TFSDuration, Min, TDuration);
            RTLINE_ACCEPTOR_MAYBE(TFSDuration, Default, TDuration);
        protected:
            virtual NJson::TJsonValue GetDefaultValueView() const override {
                if (HasDefault()) {
                    return TJsonProcessor::FormatDurationString(GetDefaultUnsafe());
                }
                return NJson::JSON_NULL;
            }

        public:
            using TBase::TBase;
            virtual EElementType GetType() const override {
                return EElementType::String;
            }

            virtual EVisualType GetVisualType() const override {
                return EVisualType::Duration;
            }

            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& path, TString& error) const override;

            virtual TString AdditionalDescription() const override;

            virtual EElementType GetRegisterType() const override {
                return EElementType::String;
            }

        private:
            static IElement::TFactory::TRegistrator<TFSDuration> Registrator;
        };

        class TFSNumeric: public IDefaultSchemeElement<double> {
        public:
            enum EVisualTypes {
                Raw /* "raw" */,
                Money /* "money" */,
                DateTime /* "timestamp" */
            };
        private:
            using TBase = IDefaultSchemeElement<double>;
            RTLINE_ACCEPTOR_MAYBE(TFSNumeric, Min, double);
            RTLINE_ACCEPTOR_MAYBE(TFSNumeric, Max, double);
            RTLINE_ACCEPTOR(TFSNumeric, Precision, ui32, 0);
            RTLINE_ACCEPTOR(TFSNumeric, Visual, EVisualTypes, EVisualTypes::Raw);
        public:
            TFSNumeric(const ui32 precision)
                : Precision(precision)
            {

            }

            using TBase::TBase;
            virtual EElementType GetType() const override {
                return EElementType::Numeric;
            }

            TFSNumeric& IsTimestamp() {
                Visual = EVisualTypes::DateTime;
                return *this;
            }

            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& path, TString& error) const override;

            virtual TString AdditionalDescription() const override;

            static IElement::TFactory::TRegistrator<TFSNumeric> Registrator;
        private:
        };
    }
}
