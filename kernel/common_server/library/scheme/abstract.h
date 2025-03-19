#pragma once
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/json/writer/json.h>
#include <library/cpp/logger/global/global.h>
#include "common.h"

namespace NCS {
    namespace NScheme {
        class IElement {
        private:
            CSA_DEFAULT(IElement, TString, FieldName);
            CSA_DEFAULT(IElement, TString, Originator);
            CSA_DEFAULT(IElement, TString, Description);
            CSA_MAYBE(IElement, ui32, OrderIdx);
            CS_ACCESS(IElement, TString, TabName, "default");
            CSA_READONLY_FLAG(Required, true);
            CSA_READONLY_FLAG(NonEmpty, false);
            CSA_FLAG(IElement, ReadOnly, false);
            CSA_MAYBE(IElement, bool, Deprecated);
            CSA_FLAG(IElement, Readable, true);
            CSA_FLAG(IElement, Writable, true);
        public:
            virtual ~IElement() = default;
            using TPtr = TAtomicSharedPtr<IElement>;
            using TFactory = NObjectFactory::TParametrizedObjectFactory<IElement, EElementType, TString>;

            IElement& SetRequired(const bool value) {
                RequiredFlag = value;
                if (!value) {
                    NonEmptyFlag = false;
                }
                return *this;
            }

            IElement& Required(const bool value = true) {
                return SetRequired(value);
            }

            IElement& SetNonEmpty(const bool value) {
                NonEmptyFlag = value;
                if (value) {
                    RequiredFlag = true;
                }
                return *this;
            }

            IElement& NonEmpty(const bool value = true) {
                return SetNonEmpty(value);
            }

            IElement& SetDeprecated() {
                Deprecated = true;
                return *this;
            }

            virtual EElementType GetType() const = 0;

            bool ValidateJson(const NJson::TJsonValue& json, const TString& path = "") const {
                TString error;
                if (!DoValidateJson(json, path, error)) {
                    if (!!error) {
                        ERROR_LOG << json.GetStringRobust() << " by path " << path << " " << error << Endl;
                    }
                    return false;
                }
                return true;
            }

            virtual bool DoValidateJson(const NJson::TJsonValue& /*json*/, const TString& /*path*/, TString& /*error*/) const {
                return false;
            }

            using TReportTraits = ui32;

            enum EReportTraits: TReportTraits {
                OrderField = 1 << 0,
                TabNameField = 1 << 1,
                ReadOnlyField = 1 << 2,
                TypeField = 1 << 3,
                InternalTypeField = 1 << 4,
                RequiredField = 1 << 5,
                DescriptionField = 1 << 6,
                PrecisionField = 1 << 7,
                AdditionalProperties = 1 << 8,
                TopLevelScheme = 1 << 9,
            };

            static TReportTraits ReportAll;
            static TReportTraits ReportFrontend;
            static TReportTraits ReportValidation;

            IElement(const TString& fieldName = "", const TString& description = "", const ui32 idx = Max<ui32>())
                : FieldName(fieldName)
                , Description(description) {
                if (idx != Max<ui32>()) {
                    OrderIdx = idx;
                }
            }

            virtual EElementType GetRegisterType() const {
                return GetType();
            }

            virtual void MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const;

            virtual NJson::TJsonValue GetDefaultValueView() const {
                return NJson::JSON_NULL;
            }

        protected:
            void PrintDescription(NJsonWriter::TBuf& buf, IOutputStream& os) const;
            void PrintValue(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const;

            virtual void AddValueToDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& /*os*/) const {
                buf.WriteJsonValue(&json, true);
            }

            virtual TString AdditionalDescription() const {
                return TString();
            }
        };

        template <class T>
        class IDefaultSchemeElement: public IElement {
        private:
            using TBase = IElement;
            using TSelf = IDefaultSchemeElement<T>;
            RTLINE_ACCEPTOR_MAYBE(TSelf, Default, T);

        public:
            using TBase::TBase;

            virtual NJson::TJsonValue GetDefaultValueView() const override {
                if (Default) {
                    return GetDefaultUnsafe();
                }
                return NJson::JSON_NULL;
            }
        };
    }
}
