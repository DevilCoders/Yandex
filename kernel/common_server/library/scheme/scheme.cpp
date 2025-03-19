#include "scheme.h"
#include <kernel/common_server/library/scheme/proto/scheme_option.pb.h>
#include <kernel/common_server/library/scheme/serialization/default.h>
#include <kernel/common_server/library/scheme/serialization/opeanapi.h>
#include <library/cpp/protobuf/json/util.h>

namespace NCS {
    namespace NScheme {

        TString TFSStructure::GetFieldName(const google::protobuf::FieldDescriptor& field, const NProtobufJson::TProto2JsonConfig& config) {
            if (config.NameGenerator) {
                return config.NameGenerator(field);
            }

            if (config.UseJsonName) {
                Y_ASSERT(!field.json_name().empty());
                TString result = field.json_name();
                if (!field.has_json_name() && !result.empty()) {
                    // FIXME: https://st.yandex-team.ru/CONTRIB-139
                    result[0] = AsciiToLower(result[0]);
                }
                return result;
            }

            switch (config.FieldNameMode) {
            case NProtobufJson::TProto2JsonConfig::FieldNameOriginalCase:
                return field.name();
            case NProtobufJson::TProto2JsonConfig::FieldNameLowerCase: {
                TString result = field.name();
                result.to_lower();
                return result;
            }

            case NProtobufJson::TProto2JsonConfig::FieldNameUpperCase: {
                TString result = field.name();
                result.to_upper();
                return result;
            }

            case NProtobufJson::TProto2JsonConfig::FieldNameCamelCase: {
                TString result = field.name();
                if (!result.empty()) {
                    result[0] = AsciiToLower(result[0]);
                }
                return result;
            }

            case NProtobufJson::TProto2JsonConfig::FieldNameSnakeCase: {
                TString result = field.name();
                NProtobufJson::ToSnakeCase(&result);
                return result;
            }

            case NProtobufJson::TProto2JsonConfig::FieldNameSnakeCaseDense: {
                TString result = field.name();
                NProtobufJson::ToSnakeCaseDense(&result);
                return result;
            }

            default:
                Y_VERIFY_DEBUG(false, "Unknown FieldNameMode.");
            }
            return TString();
        }

        TVector<TString> TFSStructure::GetVariants(const google::protobuf::EnumDescriptor& descr, const NProtobufJson::TProto2JsonConfig& config) {
            TVector<TString> result;
            for (int i = 0; i < descr.value_count(); ++i) {
                const auto& val = *descr.value(i);
                switch (config.EnumMode) {
                case NProtobufJson::TProto2JsonConfig::EnumNumber:
                    result.emplace_back(ToString(val.number()));
                    break;
                case NProtobufJson::TProto2JsonConfig::EnumName:
                    result.emplace_back(val.name());
                    break;
                case NProtobufJson::TProto2JsonConfig::EnumFullName:
                    result.emplace_back(val.full_name());
                    break;
                case NProtobufJson::TProto2JsonConfig::EnumNameLowerCase:
                    result.emplace_back(val.name());
                    result.back().to_lower();
                    break;
                case NProtobufJson::TProto2JsonConfig::EnumFullNameLowerCase:
                    result.emplace_back(val.full_name());
                    result.back().to_lower();
                    break;
                }
            }
            return result;
        }

        THolder<NCS::NScheme::IElement> TFSStructure::CreateElement(const google::protobuf::FieldDescriptor& field, const NProtobufJson::TProto2JsonConfig& config) {
            const auto& ann = field.options().GetExtension(SchemeConf);
            if (ann.VariantsSize()) {
                auto result = MakeHolder<TFSVariants>("", "");
                result->SetVariants(ann.GetVariants());
                return std::move(result);
            }
            switch (field.cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                return MakeHolder<TFSBoolean>("", "");
            case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
                auto result = MakeHolder<TFSString>("", "");
                result->SetMultiLine(ann.HasMultiline() && ann.GetMultiline());
                if (field.has_default_value()) {
                    result->SetDefault(field.default_value_string());
                }
                if (ann.HasVisual()) {
                    if (const auto* val = TSchemeOptionInfo::EVisual_descriptor()->FindValueByNumber(ann.GetVisual())) {
                        TFSString::EVisualType visual;
                        if (TryFromString(val->name(), visual)) {
                            result->SetVisual(visual);
                        }
                    }
                }
                return std::move(result);
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
                auto result = MakeHolder<TFSNumeric>("", "");
                if (field.has_default_value()) {
                    switch (field.cpp_type()) {
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        result->SetDefault(field.default_value_double());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        result->SetDefault(field.default_value_float());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        result->SetDefault(field.default_value_int32());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        result->SetDefault(field.default_value_uint32());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        result->SetDefault(field.default_value_int64());
                        break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        result->SetDefault(field.default_value_uint64());
                        break;
                    default:
                        break;
                    }
                }
                if (ann.HasVisual()) {
                    if (const auto* val = TSchemeOptionInfo::EVisual_descriptor()->FindValueByNumber(ann.GetVisual())) {
                        TFSNumeric::EVisualTypes visual;
                        if (TryFromString(val->name(), visual)) {
                            result->SetVisual(visual);
                        }
                    }
                }
                if (ann.HasMin()) {
                    result->SetMin(ann.GetMin());
                }
                if (ann.HasMax()) {
                    result->SetMax(ann.GetMax());
                }
                if (ann.HasPrecision()) {
                    result->SetPrecision(ann.GetPrecision());
                }
                return std::move(result);
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
                auto result = MakeHolder<TFSVariants>("", "");
                result->SetVariants(GetVariants(*field.enum_type(), config));
                return std::move(result);
            };
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
                if (field.is_map()) {
                    auto result = MakeHolder<TFSMap>("", "");
                    result->SetKeyType(CreateElement(*field.message_type()->map_key(), config).Release());
                    result->SetElementType(CreateElement(*field.message_type()->map_value(), config).Release());
                    return std::move(result);
                } else {
                    auto result = MakeHolder<TFSStructure>("", "");
                    result->SetProto(*field.message_type(), config);
                    return std::move(result);
                }
            }
            }
        }

        IElement::TFactory::TRegistrator<TFSStructure> TFSStructure::Registrator(EElementType::Structure);

        NJson::TJsonValue TScheme::SerializeToJson(TReportTraits traits, ESchemeFormat formatter) const {
            if (formatter == ESchemeFormat::Default) {
                TDefaultSerializer serializer(traits);
                return serializer.SerializeToJson(*this);
            } else {
                TOpenApiSerializer serializer(traits);
                return serializer.SerializeToJson(*this);
            }
        }

        NCS::NScheme::TFSStructure& TFSStructure::SetProto(const google::protobuf::Descriptor& descr, const NProtobufJson::TProto2JsonConfig& cfg /*= NProtobufJson::TProto2JsonConfig()*/) {
            if (!StructureElement) {
                SetStructure();
            }
            for (int i = 0; i < descr.field_count(); ++i) {
                const auto& field = *descr.field(i);
                const auto& ann = field.options().GetExtension(SchemeConf);
                if (ann.HasIgnore() && ann.GetIgnore()) {
                    continue;
                }
                const auto name = GetFieldName(field, cfg);
                const auto description = ann.GetDescription() ? ann.GetDescription() : name;
                const auto orderIdx = ann.HasOrderIdx() ? ann.GetOrderIdx() : Max<ui32>();
                auto element = CreateElement(field, cfg);
                element->SetReadOnly(ann.HasReadOnly() && ann.GetReadOnly());
                if (field.is_repeated()) {
                    auto& arrElement = StructureElement->Add<TFSArray>(name, description, orderIdx).SetRequired(field.is_required());
                    arrElement.SetElement(std::move(element));
                    arrElement.SetReadOnly(ann.HasReadOnly() && ann.GetReadOnly());
                    if (ann.HasMinCount()) {
                        arrElement.SetMinItems(ann.GetMinCount());
                    }
                    if (ann.HasMaxCount()) {
                        arrElement.SetMinItems(ann.GetMaxCount());
                    }
                } else {
                    StructureElement->Add(name, std::move(element), description, orderIdx).SetRequired(field.is_required());
                }
            }
            return *this;
        }

        void TFSStructure::MakeJsonDescription(const NJson::TJsonValue& json, NJsonWriter::TBuf& buf, IOutputStream& os) const {
            PrintDescription(buf, os);
            PrintValue(json, buf, os);
        }

        TStructureImpl& TStructureImpl::Remove(const TString& fName) {
            if (ElementNames.erase(fName)) {
                for (auto it = Elements.begin(); it != Elements.end();) {
                    if ((*it)->GetFieldName() == fName) {
                        it = Elements.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            return *this;
        }

        IElement::TPtr TStructureImpl::Get(const TString& fName) const {
            for (auto it = Elements.begin(); it != Elements.end(); ++it) {
                if ((*it)->GetFieldName() == fName) {
                    return *it;
                }
            }
            return nullptr;
        }

        bool TStructureImpl::ValidateJson(const NJson::TJsonValue& json, const TString& path) const {
            if (!json.IsMap()) {
                ERROR_LOG << "is not a MAP" << Endl;
                return false;
            }
            bool result = true;
            for (const auto& [k, v] : json.GetMap()) {
                if (!ElementNames.contains(k)) {
                    ERROR_LOG << "field unknown: " << k << Endl;
                    result = false;
                }
            }
            for (const auto& el : Elements) {
                const auto& name = el->GetFieldName();
                if (json.Has(name)) {
                    result &= el->ValidateJson(json[name], path + "/" + name);
                }
            }
            return result;
        }
    }
}
