#pragma once
#include "abstract.h"
#include <library/cpp/protobuf/json/config.h>
#include <util/generic/cast.h>
#include <google/protobuf/descriptor.h>

namespace NCS {
    namespace NScheme {
        class TStructureImpl {
            CSA_READONLY_DEF(TVector<IElement::TPtr>, Elements);
            CSA_READONLY_DEF(TSet<TString>, ElementNames);
        protected:
            TString CurrentTab = "default";

        public:
            using TPtr = TAtomicSharedPtr<TStructureImpl>;

            class TSchemeTabGuard {
            private:
                TStructureImpl& OwnedScheme;
            public:

                using TPtr = TAtomicSharedPtr<TSchemeTabGuard>;

                TSchemeTabGuard(const TString& tabName, TStructureImpl& ownedScheme)
                    : OwnedScheme(ownedScheme) {
                    OwnedScheme.CurrentTab = tabName;
                }

                ~TSchemeTabGuard() {
                    OwnedScheme.CurrentTab = "default";
                }
            };

            TSchemeTabGuard::TPtr StartTabGuard(const TString& tabName) {
                return MakeAtomicShared<TSchemeTabGuard>(tabName, *this);
            }

            TStructureImpl& Remove(const TString& fName);

            IElement::TPtr Get(const TString& fName) const;

            bool IsEmpty() const {
                return Elements.empty();
            }

            bool HasField(const TString& fieldName) const {
                return ElementNames.contains(fieldName);
            }

            bool ValidateJson(const NJson::TJsonValue& json, const TString& path) const;

            template <class T>
            T& Add(const TString& fieldName, const TString& description = "", const ui32 orderIdx = Max<ui32>()) {
                Y_ASSERT(!!fieldName);
                Elements.emplace_back(MakeAtomicShared<T>(fieldName, !!description ? description : fieldName, orderIdx));
                CHECK_WITH_LOG(ElementNames.emplace(fieldName).second) << fieldName;
                T* result = VerifyDynamicCast<T*>(Elements.back().Get());
                result->SetTabName(CurrentTab);
                return *result;
            }

            template <class T>
            T& Add(T&& element, const ui32 orderIdx = Max<ui32>()) {
                element.SetOrderIdx(orderIdx);
                Elements.emplace_back(MakeAtomicShared<T>(std::move(element)));
                CHECK_WITH_LOG(ElementNames.emplace(Elements.back()->GetFieldName()).second) << Elements.back()->GetFieldName() << Endl;
                T* result = VerifyDynamicCast<T*>(Elements.back().Get());
                result->SetTabName(CurrentTab);
                return *result;
            }

            IElement& Add(const TString& fieldName, THolder<IElement>&& element, const TString& description = "", const ui32 orderIdx = Max<ui32>()) {
                Y_ASSERT(!!fieldName);
                CHECK_WITH_LOG(!!element);
                element->SetFieldName(fieldName);
                element->SetDescription(description);
                element->SetOrderIdx(orderIdx);
                Elements.emplace_back(element.Release());
                CHECK_WITH_LOG(ElementNames.emplace(fieldName).second) << fieldName << Endl;
                auto* result = Elements.back().Get();
                result->SetTabName(CurrentTab);
                return *result;
            }
        };

        class TFSStructure: public IElement {
        public:
            enum class EVisualType {
                Default,
                Table
            };
        private:
            CS_ACCESS(TFSStructure, EVisualType, VisualType, EVisualType::Default);
            CSA_PROTECTED_DEF(TFSStructure, TStructureImpl::TPtr, StructureElement);
        private:
            using TBase = IElement;
        public:
            using TBase::TBase;
            TFSStructure& SetRequired(const bool value) {
                IElement::SetRequired(value);
                return *this;
            }

            virtual EElementType GetType() const override {
                return EElementType::Structure;
            }

            TStructureImpl* operator->() {
                return StructureElement.Get();
            }

            TStructureImpl& SetStructure(bool required = true) {
                SetRequired(required);
                StructureElement = MakeAtomicShared<TStructureImpl>();
                return *StructureElement.Get();
            }

            TFSStructure& SetStructure(const TStructureImpl& value, bool required = true) {
                SetRequired(required);
                StructureElement = MakeAtomicShared<TStructureImpl>(value);
                return *this;
            }

            template<class TProto>
            TFSStructure& SetProto(const NProtobufJson::TProto2JsonConfig& cfg = NProtobufJson::TProto2JsonConfig()) {
                return SetProto(*TProto::descriptor(), cfg);
            }

            TFSStructure& SetProto(const google::protobuf::Descriptor& descr, const NProtobufJson::TProto2JsonConfig& cfg = NProtobufJson::TProto2JsonConfig());

            virtual bool DoValidateJson(const NJson::TJsonValue& json, const TString& path, TString& /*error*/) const override {
                CHECK_WITH_LOG(!!StructureElement);
                return StructureElement->ValidateJson(json, path);
            }

            virtual void MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const override;

        private:
            virtual void AddValueToDescription(const NJson::TJsonValue& /*json*/, NJsonWriter::TBuf& /*buf*/, IOutputStream& /*os*/) const override {
                if (!StructureElement) {
                    ythrow yexception() << "incorrect scheme description";
                }
            }

            static TString GetFieldName(const google::protobuf::FieldDescriptor& field, const NProtobufJson::TProto2JsonConfig& config);
            static TVector<TString> GetVariants(const google::protobuf::EnumDescriptor& descr, const NProtobufJson::TProto2JsonConfig& config);
            static THolder<IElement> CreateElement(const google::protobuf::FieldDescriptor& field, const NProtobufJson::TProto2JsonConfig& config);
            static IElement::TFactory::TRegistrator<TFSStructure> Registrator;
        };

        class TScheme: public TFSStructure {
        private:
            using TBase = TFSStructure;

        public:
            using TSchemeTabGuard = TStructureImpl::TSchemeTabGuard;

            TScheme() {
                SetStructure();
            }

            TSchemeTabGuard::TPtr StartTabGuard(const TString& tabName) {
                return MakeAtomicShared<TSchemeTabGuard>(tabName, *StructureElement);
            }

            TStructureImpl& Remove(const TString& fName) {
                return StructureElement->Remove(fName);
            }

            IElement::TPtr Get(const TString& fName) const {
                return StructureElement->Get(fName);
            }

            bool IsEmpty() const {
                return StructureElement->IsEmpty();
            }

            bool HasField(const TString& fieldName) const {
                return StructureElement->HasField(fieldName);
            }

            template <class T>
            T& Add(const TString& fieldName, const TString& description = "", const ui32 orderIdx = Max<ui32>()) {
                return StructureElement->Add<T>(fieldName, description, orderIdx);
            }

            template <class T>
            T& Add(T&& element, const ui32 orderIdx = Max<ui32>()) {
                return StructureElement->Add<T>(std::move(element), orderIdx);
            }

            IElement& Add(const TString& fieldName, THolder<IElement>&& element, const TString& description = "", const ui32 orderIdx = Max<ui32>()) {
                return StructureElement->Add(fieldName, std::move(element), description, orderIdx);
            }

            operator TStructureImpl& () const {
                return *StructureElement;
            }

            NJson::TJsonValue SerializeToJson(TReportTraits traits = ReportAll,
                ESchemeFormat formatter = ESchemeFormat::Default) const;
        };
    }
}
