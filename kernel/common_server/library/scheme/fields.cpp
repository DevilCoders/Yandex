#include "fields.h"

namespace NCS {
    namespace NScheme {

        IElement::TFactory::TRegistrator<TFSIgnore> TFSIgnore::Registrator(EElementType::Ignore);
        IElement::TFactory::TRegistrator<TFSString> TFSString::Registrator(EElementType::String);
        IElement::TFactory::TRegistrator<TFSArray> TFSArray::Registrator(EElementType::Array);
        IElement::TFactory::TRegistrator<TFSBoolean> TFSBoolean::Registrator(EElementType::Boolean);
        IElement::TFactory::TRegistrator<TFSNumeric> TFSNumeric::Registrator(EElementType::Numeric);
        IElement::TFactory::TRegistrator<TFSVariants> TFSVariants::Registrator(EElementType::Variants);
        IElement::TFactory::TRegistrator<TFSWideVariants> TFSWideVariants::Registrator(EElementType::WideVariants);

        void TFSIgnore::MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const {
            if (json.IsArray() || json.IsMap()) {
                PrintDescription(buf, os);
                PrintValue(json, buf, os);
            } else {
                PrintValue(json, buf, os);
                PrintDescription(buf, os);
            }
        }

        bool TFSString::DoValidateJson(const NJson::TJsonValue& json, const TString& /*path*/, TString& error) const {
            if (!json.IsString()) {
                error = "is not a STRING";
                return false;
            }
            if (Visual != EVisualType::Unknown) {
                TGUID guid;
                bool rightType = false;
                switch (Visual) {
                    case EVisualType::GUID:
                        rightType = GetGuid(json.GetString(), guid);
                        break;
                    case EVisualType::UUID:
                        rightType = GetUuid(json.GetString(), guid);
                        break;
                    default:
                        break;
                }
                if (!rightType) {
                    error = "is not a " + ::ToString(Visual);
                    return false;
                }
            }
            return true;
        }

        bool TFSVariants::DoValidateJson(const NJson::TJsonValue& json, const TString& /*path*/, TString& error) const {
            if (MultiSelect) {
                if (!json.IsArray()) {
                    error = "is not an ARRAY";
                    return false;
                }
                const auto& jsonArray = json.GetArray();
                for (size_t i = 0; i < jsonArray.size(); ++i) {
                    const auto& elem = jsonArray[i];
                    if (!elem.IsString()) {
                        error = "[" + ToString(i) + "] is not a STRING";
                        return false;
                    }
                    if (!Editable && !Variants.contains(elem.GetString())) {
                        error = "[" + ToString(i) + "]" + elem.GetString() + " not in " + JoinSeq(", ", Variants);
                        return false;
                    }
                }
            } else {
                if (!json.IsString()) {
                    error = "is not a STRING";
                    return false;
                }
                if (!Editable && !Variants.contains(json.GetString())) {
                    error = "not in " + JoinSeq(", ", Variants);
                    return false;
                }
            }
            return true;
        }

        TString TFSVariants::AdditionalDescription() const {
            return "(variants: " + JoinSeq(", ", Variants) + ")";
        }

        bool TFSDuration::DoValidateJson(const NJson::TJsonValue& json, const TString& /*path*/, TString& error) const {
            if (!json.IsString()) {
                error = "is not a STRING";
                return false;
            }
            TDuration result;
            if (!TDuration::TryParse(json.GetString(), result)) {
                error = "is not a Duration";
                return false;
            }
            if (!!Min && result < Min) {
                error = ToString(result) + " < " + ToString(Min);
                return false;
            }
            if (!!Max && result > Max) {
                error = ToString(result) + " > " + ToString(Max);
                return false;
            }
            return true;
        }

        TString TFSDuration::AdditionalDescription() const {
            if (Min || Max) {
                return "(" + (!!Min ? " min: " + TJsonProcessor::FormatDurationString(*Min) : "") + (!!Max ? " max: " + TJsonProcessor::FormatDurationString(*Max) : "") + ")";
            }
            return TString();
        }

        TString TFSNumeric::AdditionalDescription() const {
            if (Min || Max) {
                return "(" + (!!Min ? " min: " + ToString(*Min) : "") + (!!Max ? " max: " + ToString(*Max) : "") + ")";
            }
            return TString();
        }

        bool TFSNumeric::DoValidateJson(const NJson::TJsonValue& json, const TString& /*path*/, TString& error) const {
            if (!json.IsDouble()) {
                error = "is not a Double";
                return false;
            }

            double result = json.GetDouble();
            if (!!Min && result < Min) {
                error = ToString(result) + " < " + ToString(Min);
                return false;
            }
            if (!!Max && result > Max) {
                error = ToString(result) + " > " + ToString(Max);
                return false;
            }
            if ((Visual == EVisualTypes::DateTime || Visual == EVisualTypes::Money) && result < 0) {
                error = ToString(result) + " < 0";
                return false;
            }
            return true;
        }

        bool TFSArray::DoValidateJson(const NJson::TJsonValue& json, const TString& path, TString& error) const {
            if (!json.IsArray()) {
                error = "is not an ARRAY";
                return false;
            }
            CHECK_WITH_LOG(!!ArrayTypeElement);
            const auto& jsonArray = json.GetArray();
            if (HasMinItems() && jsonArray.size() < MinItems) {
                error = TString("ARRAY items count must be not less then ") + GetMinItemsUnsafe() + ", but " + jsonArray.size() + "items was present";
                return false;
            }
            if (HasMaxItems() && jsonArray.size() < MaxItems) {
                error = TString("ARRAY items count must be not greater then ") + GetMaxItemsUnsafe() + ", but " + jsonArray.size() + "items was present";
                return false;
            }
            for (size_t i = 0; i < jsonArray.size(); ++i) {
                if (!ArrayTypeElement->ValidateJson(jsonArray[i], path + "/[" + ToString(i) + "]")) {
                    return false;
                }
            }
            return true;
        }

        void TFSArray::MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const {
            PrintDescription(buf, os);
            PrintValue(json, buf, os);
        }

        void TFSArray::AddValueToDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const {
            if (!ArrayTypeElement) {
                ythrow yexception() << "incorrect scheme: unknown element type";
            }
            buf.BeginList();
            if (!json.GetArray().empty()) {
                ArrayTypeElement->MakeJsonDescription(json.GetArray().front(), buf, os);
                if (json.GetArray().size() > 1) {
                    buf.FlushTo(&os);
                    os << "," << Endl;
                    for (size_t i = 1; i < buf.State().Stack.size(); ++i) {
                        os << "  ";
                    }
                    os << "...";
                }
            }
            buf.EndList();
        }

        bool TFSWideVariants::DoValidateJson(const NJson::TJsonValue& json, const TString& /*path*/, TString& error) const {
            TSet<TString> variantsValues;
            for (auto&& i : Variants) {
                variantsValues.emplace(i.GetRealValue());
            }
            if (MultiSelect) {
                if (!json.IsArray()) {
                    error = "is not an ARRAY";
                    return false;
                }
                const auto& jsonArray = json.GetArray();
                for (size_t i = 0; i < jsonArray.size(); ++i) {
                    const auto& elem = jsonArray[i];
                    if (!elem.IsString()) {
                        error = "[" + ToString(i) + "] is not a STRING";
                        return false;
                    }
                    if (!variantsValues.contains(elem.GetString())) {
                        error = "[" + ToString(i) + "]" + elem.GetString() + " not in " + JoinSeq(", ", variantsValues);
                        return false;
                    }
                }
            } else {
                if (!json.IsString()) {
                    error = "is not a STRING";
                    return false;
                }
                if (!variantsValues.contains(json.GetString())) {
                    error = "not in " + JoinSeq(", ", variantsValues);
                    return false;
                }
            }
            return true;
        }

    }
}
