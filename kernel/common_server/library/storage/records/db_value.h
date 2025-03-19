#pragma once
#include <variant>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/map.h>

namespace NCS {
    namespace NStorage {
        enum class EColumnType {
            Text = 1 /* "text" */,
            I32 = 2 /* "i32" */,
            I64 = 3 /* "i64" */,
            UI64 = 4 /* "ui64" */,
            Double = 6 /* "dbl" */,
            Binary = 7 /* "binary" */,
            Boolean = 8 /* "bool" */,
        };

        using TDBValue = std::variant<bool, i64, ui64, double, TString>;

        class TDBValueOperator {
        public:
            template <class T>
            static TDBValue Build(const T value) {
                return value;
            }

            static TDBValue Build(const ui32 value) {
                return (ui64)value;
            }

            static TDBValue Build(const i32 value) {
                return (i64)value;
            }

            template <class TProto>
            class TProtoSaver {
            private:
                TProto& Proto;
            public:
                TProtoSaver(TProto& proto)
                    : Proto(proto) {

                }
                void operator()(const TString& value) const {
                    Proto.SetStringValue(value);
                }
                void operator()(const ui64 value) const {
                    Proto.SetUI64Value(value);
                }
                void operator()(const i64 value) const {
                    Proto.SetI64Value(value);
                }
                void operator()(const bool value) const {
                    Proto.SetBoolValue(value);
                }
                void operator()(const double value) const {
                    Proto.SetDoubleValue(value);
                }
            };

            template <class TProto>
            static void SerializeToProto(const NCS::NStorage::TDBValue& value, TProto& proto) {
                TProtoSaver<TProto> saver(proto);
                std::visit(saver, value);
            }

            template <class TProto>
            Y_WARN_UNUSED_RESULT static bool DeserializeFromProto(NCS::NStorage::TDBValue& value, const TProto& proto) {
                if (proto.HasUI64Value()) {
                    value = proto.GetUI64Value();
                    return true;
                } else if (proto.HasI64Value()) {
                    value = proto.GetI64Value();
                    return true;
                } else if (proto.HasBoolValue()) {
                    value = proto.GetBoolValue();
                    return true;
                } else if (proto.HasDoubleValue()) {
                    value = proto.GetDoubleValue();
                    return true;
                } else if (proto.HasStringValue()) {
                    value = proto.GetStringValue();
                    return true;
                } else if (proto.HasValue()) {
                    value = proto.GetValue();
                    return true;
                } else {
                    return false;
                }
            }

            static TMaybe<EColumnType> FromJsonType(const NJson::EJsonValueType type);

            Y_WARN_UNUSED_RESULT static bool DeserializeFromJson(NStorage::TDBValue& value, const NJson::TJsonValue& jsonValue);
            static NJson::TJsonValue SerializeToJson(const NStorage::TDBValue& value);
            static TString SerializeToString(const NStorage::TDBValue& value);

            static TMaybe<TDBValue> ParseString(const TStringBuf sb, const EColumnType columnType);
            static bool TryGetString(const NStorage::TDBValue& value, TString& result);
        };
    }
}
