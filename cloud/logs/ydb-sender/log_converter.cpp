#include "log_converter.h"

#include <library/cpp/yson/json/json_writer.h>
#include <library/cpp/yson/node/node_io.h>

#include <util/string/printf.h>

NYdb::EPrimitiveType YdbTypeFromYtType(NYT::EValueType t) {
    switch (t) {
        case NYT::VT_INT8:
            return NYdb::EPrimitiveType::Int8;
        case NYT::VT_UINT8:
            return NYdb::EPrimitiveType::Uint8;

        case NYT::VT_INT16:
            return NYdb::EPrimitiveType::Int16;
        case NYT::VT_UINT16:
            return NYdb::EPrimitiveType::Uint16;

        case NYT::VT_INT32:
            return NYdb::EPrimitiveType::Int32;
        case NYT::VT_UINT32:
            return NYdb::EPrimitiveType::Uint32;

        case NYT::VT_INT64:
            return NYdb::EPrimitiveType::Int64;
        case NYT::VT_UINT64:
            return NYdb::EPrimitiveType::Uint64;

        case NYT::VT_BOOLEAN:
            return NYdb::EPrimitiveType::Bool;

        case NYT::VT_DOUBLE:
            return NYdb::EPrimitiveType::Double;

        case NYT::VT_STRING:
            return NYdb::EPrimitiveType::String;
        case NYT::VT_UTF8:
            return NYdb::EPrimitiveType::Utf8;

            // Save any as Json
        case NYT::VT_ANY:
            return NYdb::EPrimitiveType::Json;
        default:
            Y_FAIL("Unsupported type");
    }
    return NYdb::EPrimitiveType::Int64;
}

TString ParamName(TString colName) {
    return "_p_" + colName;
}

TLogConverter::TLogConverter(TString topic, TDuration newTableInterval, TString timestampColumn)
        : NewTableInterval(newTableInterval), TimestampColumn(timestampColumn) {
    NLogFeller::NParsing::UseEmbeddedConfigs();
    Splitter = NLogFeller::NChunkSplitter::CreateChunkSplitter("\n");
    Parser = NLogFeller::NLogParser::GetLogParser(topic);
    Schema = Parser->GetLogSchema();

    for (const auto &col : Schema.Columns()) {
        Columns[col.Name()] = TColInfo{col.Name(), ParamName(col.Name()), YdbTypeFromYtType(col.Type())};
    }

    if (!TimestampColumn.empty()) {
        if (!Columns.contains(TimestampColumn)) {
            ythrow yexception() << "ts column [" << TimestampColumn << "] is absent in schema";
        }

        auto &tsCol = Columns[TimestampColumn];
        if (tsCol.Type != NYdb::EPrimitiveType::Int64 && tsCol.Type != NYdb::EPrimitiveType::Uint64) {
            ythrow yexception() << "ts column [" << TimestampColumn << "] type must be Int64 or Uint64";
        }

        // We will be sorting by timestamp in reverse order
        tsCol.Type = NYdb::EPrimitiveType::Int64;
    }
}

const THashMap<TString, TLogConverter::TColInfo> &TLogConverter::GetColumnList() const {
    return Columns;
}

TString now_rfc3339() {
    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    const auto now_s = std::chrono::time_point_cast<std::chrono::seconds>(now_ms);
    const auto millis = now_ms - now_s;
    const auto c_now = std::chrono::system_clock::to_time_t(now_s);

    char buffer[30];
    auto dateLength = strftime(buffer, 20, "%FT%T", gmtime(&c_now));
    sprintf(buffer + dateLength, ".%03lldZ", millis.count());
    return buffer;
}

void TLogConverter::Parse(const NPersQueue::TReadResponse &msg,
                          THashMap<TString, TVector<TLogConverter::TLogRow>> &batches,
                          TVector<TLogConverter::TLogRow> &streams) {
    TString nowRfc3339 = now_rfc3339();
    THashSet<std::pair<TString, TString>> streamNames;
    for (ui32 i = 0; i < msg.GetData().MessageBatchSize(); ++i) {
        const auto &t = msg.GetData().GetMessageBatch(i);
        for (ui32 j = 0; j < t.MessageSize(); ++j) {
            const auto &meta = t.GetMessage(j).GetMeta();
            Y_UNUSED(meta); // TODO: extract usefull columns from meta

            auto iterator = Splitter->CreateIterator(t.GetMessage(j).GetData());

            while (iterator.Valid()) {
                TStringBuf record;
                TStringBuf skip;
                NLogFeller::NChunkSplitter::TRecordContext context;

                if (iterator.NextRecord(record, skip, context)) {
                    TString err;
                    auto row = ParseRecord(record, context, err, nowRfc3339);
                    if (!err.empty()) {
                        Cerr << "Error while parsing record: " << record << " - " << err << Endl;
                    } else {
                        streamNames.insert(extractStream(row.second));
                        TString tableName = row.first;
                        auto &res = batches[tableName];
                        res.emplace_back(std::move(row.second));
                    }
                }
            }
        }
    }

    for (auto &s: streamNames) {
        std::cerr << "Stream" << s.first << "/" << s.second << std::endl;
        streams.emplace_back(makeStreamRow(s, nowRfc3339));
    }

}

TLogConverter::TLogRow TLogConverter::makeStreamRow(std::pair<TString, TString> streamName, TString nowRfc3339) const {
    NYdb::TValueBuilder value;
    value.BeginStruct();
    value.AddMember("_p_logGroupId");
    value.BeginOptional().String(streamName.first).EndOptional();
    value.AddMember("_p_streamName");
    value.BeginOptional().String(streamName.second).EndOptional();
    value.AddMember("_p_updatedAt");
    value.BeginOptional().String(nowRfc3339).EndOptional();
    value.EndStruct();
    return value.Build();
}

std::pair<TString, TString> TLogConverter::extractStream(TLogConverter::TLogRow row) const {
    auto valueParser = NYdb::TValueParser(row);
    TString logGroup;
    TString logStream;
    valueParser.OpenStruct();
    while (valueParser.TryNextMember()) {
        const auto &paramName = valueParser.GetMemberName();
        const auto &columnName = paramName.c_str() + 3;
        valueParser.OpenOptional();
        const TString &stringValue = valueParser.GetString();
        valueParser.CloseOptional();
        if (strcmp(columnName, "logGroupId") == 0) {
            logGroup = stringValue;
        }
        if (strcmp(columnName, "streamName") == 0) {
            logStream = stringValue;
        }
    }
    const std::pair<TString, TString> &make_pair1 = std::make_pair(logGroup, logStream);
    return make_pair1;
}

TString YtNodeToJson(const NYT::TNode &node) {
    TString json;
    TStringOutput str(json);
    TString yson = NYT::NodeToYsonString(node);
    NYT::TJsonWriter writer(&str);
    writer.OnRaw(yson, ::NYson::EYsonType::Node);
    writer.Flush();
    return json;
}

void SetYdbValueFromNode(NYdb::TValueBuilder &value, const NYT::TNode &n, const NYdb::EPrimitiveType &type) {
#define SET_VALUE(valuetype, nodetype, cpptype)                              \
    {                                                                        \
        if (n.GetType() == NYT::TNode::String) {                             \
            cpptype val;                                                     \
            if (TryFromString<cpptype>(n.AsString(), val)) {                 \
                value.BeginOptional().valuetype(val).EndOptional();          \
            } else {                                                         \
                value.EmptyOptional(NYdb::EPrimitiveType::valuetype);        \
            }                                                                \
        } else {                                                             \
            value.BeginOptional().valuetype(n.As##nodetype()).EndOptional(); \
        }                                                                    \
    }

    switch (type) {
        case NYdb::EPrimitiveType::Int8: SET_VALUE(Int8, Int64, i8);
            break;
        case NYdb::EPrimitiveType::Uint8: SET_VALUE(Uint8, Uint64, ui8);
            break;

        case NYdb::EPrimitiveType::Int16: SET_VALUE(Int16, Int64, i16);
            break;
        case NYdb::EPrimitiveType::Uint16: SET_VALUE(Uint16, Uint64, ui16);
            break;

        case NYdb::EPrimitiveType::Int32: SET_VALUE(Int32, Int64, i32);
            break;
        case NYdb::EPrimitiveType::Uint32: SET_VALUE(Uint32, Uint64, ui32);
            break;

        case NYdb::EPrimitiveType::Int64: SET_VALUE(Int64, Int64, i64);
            break;
        case NYdb::EPrimitiveType::Uint64: SET_VALUE(Uint64, Uint64, ui64);
            break;

        case NYdb::EPrimitiveType::Double: SET_VALUE(Double, Double, double);
            break;

        case NYdb::EPrimitiveType::Bool:
            value.BeginOptional().Bool(n.AsBool()).EndOptional();
            break;

        case NYdb::EPrimitiveType::String:
            value.BeginOptional().String(n.AsString()).EndOptional();
            break;
        case NYdb::EPrimitiveType::Utf8:
            value.BeginOptional().Utf8(n.AsString()).EndOptional();
            break;

        case NYdb::EPrimitiveType::Json:
            value.BeginOptional().Json(YtNodeToJson(n)).EndOptional();
            break;

        default:
            Y_FAIL("Unsuported type");
            break;
    }

#undef SET_VALUE
}

TString TLogConverter::MakeTableName(TTimestamp ts, const NYT::TNode &parsed) const {
    TString suffix;
    if (parsed.HasKey("FAKE_qloud_application")) {
        // TODO: configure other fields that should be used in table name
        suffix += parsed["qloud_application"].AsString();
    } else {
        suffix = "log";
    }

    if (NewTableInterval) {
        TInstant tableTs = TInstant::Seconds(ts - (ts % NewTableInterval.Seconds()));
        std::cerr << "NewTableInterval.Seconds() " << NewTableInterval.Seconds();
        if (NewTableInterval.Seconds() % TDuration::Days(1).Seconds() == 0) {
            suffix += tableTs.FormatGmTime("__%Y-%m-%d");
        } else {
            suffix += tableTs.FormatGmTime("__%Y-%m-%d_%H:%M:%S");
        }
    }

    return suffix;
}

std::pair<TString, TLogConverter::TLogRow> TLogConverter::ParseRecord(
        TStringBuf record, NLogFeller::NChunkSplitter::TRecordContext context, TString &err,
        TString nowRfc3339) noexcept {
    NYT::TNode parsed;
    auto res = Parser->Parse(record, parsed, context);

    if (!res) {
        err = res.GetParsingErrorDescription();
        return std::make_pair(TString(), NYdb::TValueBuilder().Build());
    }

    TTimestamp ts = TimestampColumn ? parsed[TimestampColumn].IntCast<i64>() : Parser->GetRecordTimestamp(parsed);

    TString tableName = MakeTableName(ts, parsed);

    NYdb::TValueBuilder value;
    value.BeginStruct();

    for (const auto &col : Schema.Columns()) {
        if (col.Name() == "_logfeller_timestamp" || col.Name() == "iso_eventtime") {
            continue;
        }
        const TColInfo *colInfo = Columns.FindPtr(col.Name());
        value.AddMember(colInfo->ParamName);

        if (!TimestampColumn.empty() && col.Name() == TimestampColumn) {
            // Sort by timestamp in reverse order so that newest log rows are always first
            value.OptionalInt64(-ts);
        } else if (parsed.HasKey(col.Name())) {
            NYT::TNode &n = parsed[col.Name()];
            if (n.IsString() || n.IsInt64() || n.IsUint64() || n.IsDouble() || n.IsMap()) {
                SetYdbValueFromNode(value, parsed[col.Name()], colInfo->Type);
            } else {
                Cerr << "UNSUPPORTED TYPE, col: " << col.Name() << " " << n.GetType() << Endl;

                value.EmptyOptional(colInfo->Type);
            }
        } else {
            value.EmptyOptional(colInfo->Type);
        }
        // set or override
        if (col.Name() == "savedAt") {
            SetYdbValueFromNode(value, NYT::TNode(nowRfc3339), colInfo->Type);
        }
    }

    value.EndStruct();
    return std::make_pair(tableName, value.Build());
}
