#include "yt.h"

#include <mapreduce/yt/interface/serialize.h>

#include <kernel/common_server/library/json/adapters.h>
#include <kernel/common_server/library/json/parse.h>
#include <kernel/common_server/library/yt/node/cast.h>

namespace {
    using namespace NYT;

    const TString NameFieldName = "name";
    const TString TypeFieldName = "type";

    template <class T>
    auto GetSchemaJsonAdapter(T&& schema) {
        return NJson::KeyValue(std::forward<T>(schema), NameFieldName, TypeFieldName);
    }

    auto GetSchemaTypes() {
        TVector<TString> result;
        for (auto&& i : {
            VT_INT64,
            VT_UINT64,
            VT_DOUBLE,
            VT_BOOLEAN,
            VT_STRING,
            VT_ANY,
            VT_INT8,
            VT_INT16,
            VT_INT32,
            VT_UINT8,
            VT_UINT16,
            VT_UINT32,
            VT_UTF8,
        }) {
            result.push_back(NYT::NDetail::ToString(i));
        }
        return result;
    }

    bool TryDeserialize(EValueType& valueType, const TNode& node) try {
        Deserialize(valueType, node);
        return true;
    } catch (...) {
        ERROR_LOG << "cannot deserialize NYT::EValueType from node" << Endl;
        return false;
    }
}

template <>
NJson::TJsonValue NJson::ToJson(const NYT::EValueType& object) {
    return NYT::NDetail::ToString(object);
}

template <>
bool NJson::TryFromJson(const NJson::TJsonValue& value, NYT::EValueType& result) {
    TString intermediate;
    return NJson::TryFromJson(value, intermediate) && TryDeserialize(result, NYT::TNode(intermediate));
}

NYT::TTableSchema TYtProcessTraits::GetYtSchema(const TSchema& schema) {
    NYT::TTableSchema result;
    for (auto&&[name, type] : schema) {
        result.AddColumn(name, type);
    }
    return result;
}

NYT::TTableSchema TYtProcessTraits::GetYtSchema() const {
    return YtSchema;
}

bool TYtProcessTraits::HasYtSchema() const {
    return !YtSchema.Empty();
}

NYT::TNode TYtProcessTraits::Schematize(NYT::TNode&& record, const NYT::TTableSchema& schema) const {
    if (schema.Empty()) {
        return record;
    }

    NYT::TNode result;
    for (auto&& column : schema.Columns()) {
        const auto& name = column.Name();
        const auto type = column.Type();
        auto gLogging = TFLRecords::StartContext()("column_name", column.Name());
        auto& value = record[name];
        try {
            if (!value.HasValue() || value == "") {
                Y_ENSURE(!column.Required(), "column " << name << " is missing");
                result[name] = NYT::TNode::CreateEntity();
                continue;
            }
            switch (type) {
                case NYT::VT_ANY:
                    result[name] = std::move(value);
                    break;
                case NYT::VT_BOOLEAN:
                    result[name] = value.ConvertTo<bool>();
                    break;
                case NYT::VT_DOUBLE:
                    result[name] = value.ConvertTo<double>();
                    break;
                case NYT::VT_INT16:
                case NYT::VT_INT32:
                case NYT::VT_INT64:
                case NYT::VT_INT8:
                    result[name] = value.ConvertTo<i64>();
                    break;
                case NYT::VT_UINT16:
                case NYT::VT_UINT32:
                case NYT::VT_UINT64:
                case NYT::VT_UINT8:
                    result[name] = value.ConvertTo<ui64>();
                    break;
                case NYT::VT_STRING:
                case NYT::VT_UTF8:
                    result[name] = value.ConvertTo<TString>();
                    break;
                case NYT::VT_DATE:
                case NYT::VT_DATETIME:
                case NYT::VT_TIMESTAMP:
                case NYT::VT_INTERVAL:
                case NYT::VT_VOID:
                case NYT::VT_NULL:
                case NYT::VT_JSON:
                case NYT::VT_FLOAT:
                    ythrow yexception() << "unsupported type: " << type;
            }
        } catch (...) {
            TFLEventLog::Log("record parsing failed")("exception", CurrentExceptionMessage());
            throw;
        }
    }
    return result;
}

NYT::TNode TYtProcessTraits::Schematize(NYT::TNode&& record) const {
    return Schematize(std::move(record), YtSchema);
}

void TYtProcessTraits::FillScheme(NFrontend::TScheme& scheme) const {
    NFrontend::TScheme& schema = scheme.Add<TFSArray>("yt_schema", "YT Schema").SetElement<NFrontend::TScheme>();
    schema.Add<TFSString>(NameFieldName, "Column name").SetRequired(true);
    schema.Add<TFSVariants>(TypeFieldName, "Column type").SetVariants(GetSchemaTypes());
}

void TYtProcessTraits::Serialize(NJson::TJsonValue& value) const {
    value["yt_schema"] = NJson::ToJson(GetSchemaJsonAdapter(Schema));
}

bool TYtProcessTraits::Deserialize(const NJson::TJsonValue& value) {
    if (!NJson::ParseField(value["yt_schema"], GetSchemaJsonAdapter(Schema))) {
        TFLEventLog::Alert("failed to parse yt_schema");
        return false;
    }
    YtSchema = GetYtSchema(Schema);
    return true;
}
