#include "converter.h"

#include "util/string/builder.h"
#include "util/string/cast.h"
#include "util/string/type.h"
#include "util/system/compiler.h"

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/generic/string.h>
#include <library/cpp/logger/global/global.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

namespace NProtobufXml {

    static const TString& GetXmlFieldName(const google::protobuf::FieldDescriptor& field) {
        if (field.is_extension()) {
            if (field.containing_type()->options().message_set_wire_format() && field.type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE && field.is_optional() && field.extension_scope() == field.message_type()) {
                return field.message_type()->full_name();
            } else {
                return field.full_name();
            }
        } else {
            if (field.type() == google::protobuf::FieldDescriptor::TYPE_GROUP) {
                return field.message_type()->name();
            } else {
                return field.name();
            }
        }
    }

    static void Proto2XmlFieldValue(const google::protobuf::Message& message, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field, int field_index, NXml::TNode& node) {
        GOOGLE_DCHECK(field.is_repeated() || (field_index == -1)) << "field_index must be -1 for non-repeated fields";

        switch (field.cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
                int32_t number = field.is_repeated() ? reflection.GetRepeatedInt32(message, &field, field_index) : reflection.GetInt32(message, &field);
                node.AddChild(GetXmlFieldName(field), number);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
                int64_t number = field.is_repeated() ? reflection.GetRepeatedInt64(message, &field, field_index) : reflection.GetInt64(message, &field);
                node.AddChild(GetXmlFieldName(field), number);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
                uint32_t number = field.is_repeated() ? reflection.GetRepeatedUInt32(message, &field, field_index) : reflection.GetUInt32(message, &field);
                node.AddChild(GetXmlFieldName(field), number);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
                uint64_t number = field.is_repeated() ? reflection.GetRepeatedUInt64(message, &field, field_index) : reflection.GetUInt64(message, &field);
                node.AddChild(GetXmlFieldName(field), number);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
                float number = field.is_repeated() ? reflection.GetRepeatedFloat(message, &field, field_index) : reflection.GetFloat(message, &field);
                node.AddChild(GetXmlFieldName(field), number);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
                double number = field.is_repeated() ? reflection.GetRepeatedDouble(message, &field, field_index) : reflection.GetDouble(message, &field);
                node.AddChild(GetXmlFieldName(field), number);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
                const TString stringValue = field.is_repeated() ? reflection.GetRepeatedString(message, &field, field_index) : reflection.GetString(message, &field);
                node.AddChild(GetXmlFieldName(field), stringValue);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
                TString value("false");
                if (field.is_repeated()) {
                    if (reflection.GetRepeatedBool(message, &field, field_index)) {
                        value = "true";
                    }
                } else {
                    if (reflection.GetBool(message, &field)) {
                        value = "true";
                    }
                }
                node.AddChild(GetXmlFieldName(field), value);
                break;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
                TString value = field.is_repeated() ? reflection.GetRepeatedEnum(message, &field, field_index)->name() : reflection.GetEnum(message, &field)->name();
                node.AddChild(GetXmlFieldName(field), value);
                break;
            }
            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
                auto child = node.AddChild(field.name());
                TConverter::ProtoToXml(field.is_repeated() ? reflection.GetRepeatedMessage(message, &field, field_index) : reflection.GetMessage(message, &field), child);
                break;
            }
        }
    }

    static void Proto2XmlField(const google::protobuf::Message& message, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field, NXml::TNode& node) {
        int count = 0;
        if (field.is_repeated()) {
            count = reflection.FieldSize(message, &field);
        } else if (reflection.HasField(message, &field)) {
            count = 1;
        }

        for (int j = 0; j < count; ++j) {
            int field_index = j;
            if (!field.is_repeated()) {
                field_index = -1;
            }

            Proto2XmlFieldValue(message, reflection, field, field_index, node);
        }
    }

    static void FormatEnumValue(TString str) {
        for (TString::iterator i = str.begin(); i != str.end(); ++i) {
            if (isalnum(*i)) {
                *i = toupper(*i);
            } else {
                *i = '_';
            }
        }
    }

    static bool ParseSimpleField(google::protobuf::Message& message, const google::protobuf::Reflection& reflection, const google::protobuf::FieldDescriptor& field, const TString& value) {
        switch (field.cpp_type()) {
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32: {
                char* endptr = NULL;
                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddInt32(&message, &field, strtol(value.c_str(), &endptr, 0));
                } else {
                    reflection.SetInt32(&message, &field, strtol(value.c_str(), &endptr, 0));
                }
                if (*endptr != '\0') {
                    ERROR_LOG << "Expected int32 value for " << field.full_name() << " but got \"" << value << "\"." << Endl;
                    return false;
                }
                return true;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_INT64: {
                char* endptr = NULL;
                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddInt64(&message, &field, strtoll(value.c_str(), &endptr, 0));
                } else {
                    reflection.SetInt64(&message, &field, strtoll(value.c_str(), &endptr, 0));
                }
                if (*endptr != '\0') {
                    ERROR_LOG << "Expected int64 value for " << field.full_name() << " but got \"" << value << "\"." << Endl;
                    return false;
                }
                return true;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32: {
                char* endptr = NULL;
                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddUInt32(&message, &field, strtoul(value.c_str(), &endptr, 0));
                } else {
                    reflection.SetUInt32(&message, &field, strtoul(value.c_str(), &endptr, 0));
                }
                if (*endptr != '\0') {
                    ERROR_LOG << "Expected uint32 value for " << field.full_name() << " but got \"" << value << "\"." << Endl;
                    return false;
                }
                return true;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64: {
                char* endptr = NULL;
                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddUInt64(&message, &field, strtoull(value.c_str(), &endptr, 0));
                } else {
                    reflection.SetUInt64(&message, &field, strtoull(value.c_str(), &endptr, 0));
                }
                if (*endptr != '\0') {
                    ERROR_LOG << "Expected uint64 value for " << field.full_name() << " but got \"" << value << "\"." << Endl;
                    return false;
                }
                return true;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE: {
                char* endptr = NULL;
                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddDouble(&message, &field, strtod(value.c_str(), &endptr));
                } else {
                    reflection.SetDouble(&message, &field, strtod(value.c_str(), &endptr));
                }
                if (*endptr != '\0') {
                    ERROR_LOG << "Expected double value for " << field.full_name() << " but got \"" << value << "\"." << Endl;
                    return false;
                }
                return true;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT: {
                char* endptr = NULL;
                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddFloat(&message, &field, strtof(value.c_str(), &endptr));
                } else {
                    reflection.SetFloat(&message, &field, strtof(value.c_str(), &endptr));
                }
                if (*endptr != '\0') {
                    ERROR_LOG << "Expected float value for " << field.full_name() << " but got \"" << value << "\"." << Endl;
                    return false;
                }
                return true;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL: {
                bool isTrue = IsTrue(value);
                bool isFalse = (IsFalse(value) || value == "");
                if (!isTrue && !isFalse) {
                    ERROR_LOG << "Expected bool value for " << field.full_name() << " but got \"" << value << "\"." << Endl;
                    return false;
                } else if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddBool(&message, &field, isTrue);
                } else {
                    reflection.SetBool(&message, &field, isTrue);
                }
                return true;
            }

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddString(&message, &field, value);
                } else {
                    reflection.SetString(&message, &field, value);
                }
                return true;

            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
                TString enumValueName = field.enum_type()->name();
                enumValueName += '_';
                enumValueName += value;
                FormatEnumValue(enumValueName);
                const google::protobuf::EnumValueDescriptor* enumValue =
                    field.enum_type()->FindValueByName(enumValueName);

                if (enumValue == NULL) {
                    ERROR_LOG << field.enum_type()->full_name() << " has no value named \"" << enumValueName << "\".";
                    return false;
                }

                if (field.label() == google::protobuf::FieldDescriptor::LABEL_REPEATED) {
                    reflection.AddEnum(&message, &field, enumValue);
                } else {
                    reflection.SetEnum(&message, &field, enumValue);
                }
                return true;
            }

            default:
                ERROR_LOG << "Don't know how to parse simple field: " << field.full_name() << Endl;
                return false;
        }
    }

    static void ParseNodeAttributes(const NXml::TConstNode& node, google::protobuf::Message& message) {
        const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
        if (!descriptor) {
            return;
        }

        const google::protobuf::Reflection* reflection = message.GetReflection();
        if (!reflection) {
            return;
        }

        for (int i = 0; i < descriptor->field_count(); ++i) {
            const google::protobuf::FieldDescriptor* field = descriptor->field(i);
            if (!field) {
                continue;
            }
            if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
                continue;
            }
            auto value = node.Attr<TString>(field->name(), "");
            if (!value) {
                continue;
            }
            if (!ParseSimpleField(message, *reflection, *field, value)) {
                ERROR_LOG << "ERROR: cannot parse attribute field: \"" << field->name() << "\"." << Endl;
            }
        }
    }

    static void ParseNode(const NXml::TConstNode& node, google::protobuf::Message& message, const bool isMessage, const bool autodetectAttributes) {
        if (node.IsNull()) {
            return;
        }
        if (node.IsText()) {
            TString value = node.Value<TString>("");
            if (value == "") {
                return;
            }
            TString name = node.Parent().Name();
            if (isMessage && autodetectAttributes) {
                name = "Value";
            }

            const google::protobuf::Reflection* reflection = message.GetReflection();
            if (!reflection) {
                return;
            }

            const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
            if (!descriptor) {
                return;
            }

            const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(name);
            if (!field) {
                ERROR_LOG << descriptor->full_name() << " has no field \"" << name << "\"." << Endl;
                return;
            }

            if (!ParseSimpleField(message, *reflection, *field, value)) {
                ERROR_LOG << "ERROR: cannot parse simple field: \"" << name << "\"." << Endl;
            }
        }
        if (node.IsElementNode()) {
            TString name = node.Name();

            const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
            if (!descriptor) {
                return;
            }

            const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(name);
            if (!field) {
                ERROR_LOG << descriptor->full_name() << " has no field \"" << name << "\"." << Endl;
                return;
            }

            const google::protobuf::Reflection* reflection = message.GetReflection();
            if (!reflection) {
                return;
            }

            google::protobuf::Message* protoChild = &message;
            const bool isChildMessage = (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE);
            if (isChildMessage && field->is_repeated()) {
                protoChild = reflection->AddMessage(&message, field);
            } else if (isChildMessage) {
                protoChild = reflection->MutableMessage(&message, field);
            }

            auto xmlChildrenNode = node.FirstChild();
            while (!xmlChildrenNode.IsNull()) {
                ParseNode(xmlChildrenNode, *protoChild, isChildMessage, autodetectAttributes);
                xmlChildrenNode = xmlChildrenNode.NextSibling();
            }

            if (autodetectAttributes && isChildMessage) {
                ParseNodeAttributes(node, *protoChild);
            }
        }
    }

    void TConverter::ProtoToXml(const google::protobuf::Message& message, NXml::TNode& node) {
        const google::protobuf::Reflection* reflection = message.GetReflection();
        if (!reflection) {
            return;
        }

        TVector<const google::protobuf::FieldDescriptor*> fields;
        reflection->ListFields(message, &fields);
        for (unsigned int i = 0; i < fields.size(); i++) {
            if (!fields[i]) {
                continue;
            }
            Proto2XmlField(message, *reflection, *fields[i], node);
        }
    }

    void TConverter::XmlToProto(const NXml::TConstNode& node, google::protobuf::Message& message, const bool autodetectAttributes) {
        if (node.IsNull()) {
            return;
        }
        auto child = node.FirstChild();
        while (!child.IsNull()) {
            ParseNode(child, message, true, autodetectAttributes);
            child = child.NextSibling();
        }

        if (autodetectAttributes) {
            ParseNodeAttributes(node, message);
        }
    }

}
