#pragma once

#if !defined(INCLUDE_SCHEMA_IMPL_H)
#error "you should never include schema-impl.h directly"
#endif

#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NUgc {
    using TProto = google::protobuf::Message;
    using TSchemaValue = NJson::TJsonValue;

    struct TSchema {
        TSchemaValue Value;

        TSchema(TStringBuf s) {
            ReadJsonTree(s, &Value, true);
        }
    };

    class TSegmentSplitter {
        static constexpr char DELIM = '/';
        TStringBuf Rest;

    public:
        TSegmentSplitter(TStringBuf s)
            : Rest(s)
        {
        }

        TStringBuf NextSegment() {
            if (Rest.empty()) {
                return TStringBuf();
            }

            Y_ENSURE(Rest[0] == DELIM, "no leading delimiter");
            auto pos = Rest.find(DELIM, 1);
            return Rest.NextTokAt(pos);
        }
    };

    template <typename TSchema, typename TEntity>
    TEntity CreateSchemaEntity(TStringBuf path) {
        TEntity entity;

        const TSchemaValue& schema = Singleton<TSchema>()->Value;
        const TSchemaValue* curTable = &schema;
        TString curTableName;

        auto splitter = TSegmentSplitter(path);
        size_t consumed = 0;

        using TLocalKey = std::pair<TString, TString>;
        TVector<TLocalKey> localKeys;

        for (;;) {
            // match inner table by key prefix
            TStringBuf prefix = splitter.NextSegment();
            TStringBuilder key;
            for (const auto& tabIt : (*curTable)["TABLE"].GetMap()) {
                const auto& table = tabIt.second;
                for (const auto& keyIt : table["KEYSPACE"].GetMap()) {
                    const auto& keyspace = keyIt.second;
                    if (keyIt.first == prefix.Tail(1)) {
                        // consume key segments
                        consumed += prefix.size();
                        bool noprefix = keyspace["NOPREFIX"].GetBoolean();
                        if (!noprefix) {
                            key << prefix;
                        }

                        int segments = FromString(keyspace["SEGMENTS"].GetString());
                        for (int i = 0; i < segments; ++i) {
                            TStringBuf segment = splitter.NextSegment();
                            Y_ENSURE(segment.size(), "empty key segment");
                            consumed += segment.size();
                            if (noprefix) {
                                segment.Skip(1);
                            }
                            key << segment;
                        }

                        const TString& keyField = table["KEY"].GetString();
                        localKeys.push_back({keyField, key});
                        curTable = &table;
                        curTableName = tabIt.first;
                        break;
                    }
                }
                if (!key.empty()) {
                    break;
                }
            }

            if (key.empty()) {
                break;
            }
        }

        Y_ENSURE(curTable->Has("ROW"), "entity not found: " << path);
        const TString& rowName = curTableName;

        TProto* rowProto = FindEntityRow(entity, rowName);
        Y_ENSURE(rowProto, "row not found: " << rowName);

        auto descriptor = rowProto->GetDescriptor();
        auto reflection = rowProto->GetReflection();

        for (const auto& key: localKeys) {
            auto keyField = descriptor->FindFieldByName(key.first);
            Y_ENSURE(keyField, "key not found: " << key.first);
            reflection->SetString(rowProto, keyField, key.second);
        }
        return entity;
    }

    static inline NJson::TJsonValue JsonToFedorson(const NJson::TJsonValue& json, bool topLevel = true) {
        NJson::TJsonValue fson;
        if (json.IsMap()) {
            for (const auto& it: json.GetMap()) {
                if (topLevel && it.first == "time") {
                    // time must remain integer
                    fson[it.first] = it.second;
                } else {
                    fson[it.first] = JsonToFedorson(it.second, false);
                }
            }
        } else if (json.IsArray()) {
            for (size_t i = 0; i < json.GetArray().size(); ++i) {
                fson[i] = JsonToFedorson(json[i], false);
            }
        } else if (json.IsBoolean()) {
            fson = json.GetBoolean() ? "yes" : "no";
        } else if (json.IsDouble()) {
            fson = FloatToString(json.GetDouble());
        } else if (json.IsInteger()) {
            fson = ToString(json.GetInteger());
        } else if (json.IsUInteger()) {
            fson = ToString(json.GetUInteger());
        } else {
            fson = json;
        }
        return fson;
    }

} // namespace NUgc
