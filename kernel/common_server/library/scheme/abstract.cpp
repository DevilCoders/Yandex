#include "abstract.h"

namespace NCS {
    namespace NScheme {
        IElement::TReportTraits IElement::ReportAll = Max<IElement::TReportTraits>();
        IElement::TReportTraits IElement::ReportFrontend = IElement::EReportTraits::OrderField | IElement::EReportTraits::TabNameField | IElement::EReportTraits::ReadOnlyField | IElement::EReportTraits::TypeField | IElement::EReportTraits::RequiredField | IElement::EReportTraits::DescriptionField | IElement::EReportTraits::PrecisionField;
        IElement::TReportTraits IElement::ReportValidation = IElement::EReportTraits::TypeField | IElement::EReportTraits::InternalTypeField | IElement::EReportTraits::RequiredField | IElement::EReportTraits::DescriptionField | IElement::EReportTraits::AdditionalProperties;

        void IElement::PrintDescription(NJsonWriter::TBuf& buf, IOutputStream& os) const {
            buf.FlushTo(&os);
            os << " // (" << ::ToString(GetRegisterType()) << (RequiredFlag ? "" : ", optional") << ")";
            if (Description && (Description != FieldName)) {
                os << " " << Description;
            }
            auto additionalDescription = AdditionalDescription();
            if (additionalDescription) {
                os << " " << additionalDescription;
            }
        }

        void IElement::PrintValue(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const {
            if (!ValidateJson(json)) {
                ythrow yexception() << "incorrect json";
            }
            AddValueToDescription(json, buf, os);
        }

        void IElement::MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const {
            PrintValue(json, buf, os);
            PrintDescription(buf, os);
        }
    }
}
