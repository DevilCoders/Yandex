#include "schema.h"

#include <mapreduce/yt/interface/protos/extension.pb.h>
#include <kernel/yt/protos/extension.pb.h>

#include <yt/yt/core/misc/serialize.h>
#include <yt/yt/core/misc/format.h>
#include <yt/yt/core/misc/safe_assert.h>
#include <yt/yt/core/ytree/convert.h>
#include <yt/yt/core/yson/protobuf_interop.h>
#include <yt/yt/client/table_client/comparator.h>
#include <yt/yt/client/table_client/name_table.h>
#include <yt/yt/client/table_client/row_buffer.h>
#include <yt/yt/client/table_client/unversioned_value.h>

#include <library/cpp/protobuf/util/simple_reflection.h>
#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/threading/skip_list/skiplist.h>
#include <library/cpp/yt/assert/assert.h>
#include <library/cpp/yt/misc/enum.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <util/system/yassert.h>
#include <util/system/mutex.h>
#include <util/system/spinlock.h>
#include <util/system/guard.h>
#include <util/generic/xrange.h>
#include <util/generic/singleton.h>

namespace NYT {
namespace NProtoApi {

using NTableClient::ESortOrder;
using NTableClient::SystemColumnNamePrefix;
using NTableClient::TimestampColumnName;

//TODO: use FieldDescriptor::CppType
static ESimpleLogicalValueType MapFieldType(EFieldType type) {
    switch (type) {
    case EFieldType::TYPE_INT64:
    case EFieldType::TYPE_SFIXED64:
    case EFieldType::TYPE_SINT64:
        return ESimpleLogicalValueType::Int64;
    case EFieldType::TYPE_INT32:
    case EFieldType::TYPE_SFIXED32:
    case EFieldType::TYPE_SINT32:
        return ESimpleLogicalValueType::Int32;
    case EFieldType::TYPE_UINT64:
    case EFieldType::TYPE_FIXED64:
        return ESimpleLogicalValueType::Uint64;
    case EFieldType::TYPE_FIXED32:
    case EFieldType::TYPE_UINT32:
        return ESimpleLogicalValueType::Uint32;
    case EFieldType::TYPE_DOUBLE:
        return ESimpleLogicalValueType::Double;
    case EFieldType::TYPE_FLOAT:
        return ESimpleLogicalValueType::Float;
    case EFieldType::TYPE_BOOL:
        return ESimpleLogicalValueType::Boolean;
    case EFieldType::TYPE_STRING:
    case EFieldType::TYPE_BYTES:
    case EFieldType::TYPE_MESSAGE:
    case EFieldType::TYPE_ENUM:
        return ESimpleLogicalValueType::String;
    default:
        THROW_ERROR_EXCEPTION("Unsupported proto field type %u", int(type));
    }
}

static ESimpleLogicalValueType GetForcedTypeOrDefault(const ::google::protobuf::RepeatedField<EWrapperFieldFlag::Enum>& flags,
                                         ESimpleLogicalValueType defaultType)
{
    ESimpleLogicalValueType columnType = defaultType;
    for (const auto& flag : flags) {
        switch (flag) {
            case EWrapperFieldFlag::OTHER_COLUMNS:
            case EWrapperFieldFlag::ENUM_INT:
            case EWrapperFieldFlag::SERIALIZATION_PROTOBUF:
            case EWrapperFieldFlag::ENUM_STRING:
            case EWrapperFieldFlag::SERIALIZATION_YT:
            case EWrapperFieldFlag::OPTIONAL_LIST:
            case EWrapperFieldFlag::REQUIRED_LIST:
            case EWrapperFieldFlag::MAP_AS_LIST_OF_STRUCTS_LEGACY:
            case EWrapperFieldFlag::MAP_AS_LIST_OF_STRUCTS:
            case EWrapperFieldFlag::MAP_AS_OPTIONAL_DICT:
            case EWrapperFieldFlag::MAP_AS_DICT:
                break;
            case EWrapperFieldFlag::ANY:
                columnType = ESimpleLogicalValueType::Any;
                break;
            case EWrapperFieldFlag::EMBEDDED:
                Y_FAIL("Unsupported EMBEDDED flag for dynamic tables");
        }
    }
    return columnType;
}

static TStringBuf GetColumnName(TFieldDescriptorPtr field) {
    auto& options = field->options();
    if (options.HasExtension(key_column_name)) {
        return options.GetExtension(key_column_name);
    } else if (options.HasExtension(column_name)) {
        return options.GetExtension(column_name);
    } else {
        return field->name();
    }
}

static bool IsKeyColumn(TFieldDescriptorPtr field) {
    auto& options = field->options();
    return options.HasExtension(key)
        || options.HasExtension(key_column_name)
        || options.HasExtension(expression);
}

static ESimpleLogicalValueType GetColumnType(TFieldDescriptorPtr field) {
    return GetForcedTypeOrDefault(field->options().GetRepeatedExtension(NYT::flags), MapFieldType(field->type()));
}

static TFieldDescriptorPtr GetDeltaField(TFieldDescriptorPtr desc) {
    auto oneof = desc->containing_oneof();
    if (!oneof || oneof->field_count() != 2) {
        return nullptr;
    }

    TFieldDescriptorPtr opposite;
    if (oneof->field(0) == desc) {
        opposite = oneof->field(1);
    } else if (oneof->field(1) == desc) {
        opposite = oneof->field(0);
    } else {
        Y_FAIL();
    }

    if (opposite->options().HasExtension(aggregate)) {
        return opposite;
    }

    return nullptr;
}

TString TProtoSchema::ReprStr() const {
    return Format("%v[%v]", Descriptor()->full_name(), Kind_);
}

TFieldDescriptorPtr TProtoSchema::FieldByColumnName(TStringBuf name) const {
    Y_ENSURE(NameToFieldTag.contains(name),
        Format("No field for column %v in %v", name, ReprStr()));
    int tag = NameToFieldTag.at(name);
    return Descriptor()->FindFieldByNumber(tag);
}

void TProtoSchema::ConvertColumnFilter(TColumnFilter& filter) const {
    if (filter.IsUniversal()) {
        return;
    }

    TColumnFilter::TIndexes indexes;
    for (int tag : filter.GetIndexes()) {
        TFieldDescriptorPtr field = Descriptor()->FindFieldByNumber(tag);
        YT_LOG_FATAL_UNLESS(field, "Bad column tag number. Use TProto::kXXXNumber");
        indexes.push_back(NameTable()->GetIdOrRegisterName(GetColumnName(field))); // thread-safe
    }

    filter = TColumnFilter(std::move(indexes));
}

void TProtoSchema::RowToMessage(
    TUnversionedRow const& row,
    NProtoBuf::Message* msg,
    TConvertRowOptions const& opts) const try
{
    THolder<NProtoBuf::Message> parsed(msg->New());

    if (opts.Partial) {
        parsed->CopyFrom(*msg);
    }

    if (!row) {
        msg->GetReflection()->Swap(msg, parsed.Get());
        return;
    }

    for (TUnversionedValue const& val : row) {
        if (val.Id >= IdToField.size()) {
            THROW_ERROR_EXCEPTION("Bad column Id: %v", val.Id);
        }

        TFieldDescriptorPtr desc = IdToField[val.Id];
        if (!desc) {
            // Allow to skip system columns in description
            continue;
        }

        if (Any(val.Flags & NYT::NTableClient::EValueFlags::Aggregate) && GetDeltaField(desc)) {
            desc = GetDeltaField(desc);
        }

        NProtoBuf::TMutableField field{*parsed, desc};
        if (val.Type != EValueType::Null && val.Type != GetPhysicalType(GetColumnType(field.Field()))) {
            THROW_ERROR_EXCEPTION("Field type mismatch: %v vs %v",
                    val.Type, GetColumnType(field.Field()));
        }

        switch (val.Type) {
            case EValueType::Int64:
                field.Set(val.Data.Int64);
                continue;
            case EValueType::Uint64:
                field.Set(val.Data.Uint64);
                continue;
            case EValueType::Double:
                field.Set(val.Data.Double);
                continue;
            case EValueType::Boolean:
                field.Set(val.Data.Boolean);
                continue;
            case EValueType::String:
                if (field.IsMessage()) {
                    if (desc->options().HasExtension(message_to_yson)) {
                        TString protobufString;
                        ::google::protobuf::io::StringOutputStream protobufOutput(&protobufString);
                        auto protobufWriter = NYT::NYson::CreateProtobufWriter(&protobufOutput, NYson::ReflectProtobufMessageType(field.MutableMessage()->GetDescriptor()));
                        NYson::ParseYsonStringBuffer({val.Data.String, val.Length}, NYson::EYsonType::Node, protobufWriter.get());
                        Y_PROTOBUF_SUPPRESS_NODISCARD field.MutableMessage()->ParseFromArray(protobufString.data(), protobufString.length());
                    } else {
                        //TODO: check return value
                        Y_PROTOBUF_SUPPRESS_NODISCARD field.MutableMessage()->ParseFromArray(val.Data.String, val.Length);
                    }
                } else if (field.IsString()) {
                    field.Set(val.AsString());
                } else if (const auto enumDesc = desc->enum_type()) {
                    if (const auto enumVal = enumDesc->FindValueByName(val.AsString())) {
                        field.Set(enumVal);
                    } else {
                        THROW_ERROR_EXCEPTION("Unknown value '%v' for enum %v", val.AsString(), enumDesc->full_name());
                    }
                } else {
                    THROW_ERROR_EXCEPTION("Unknown value type stored in string");
                }
                continue;
            case EValueType::Null:
                field.Clear();
                continue;
            case EValueType::Any:
            case EValueType::Composite:
                if (field.IsMessage()) {
                    if (desc->options().HasExtension(message_to_yson)) {
                        TString protobufString;
                        ::google::protobuf::io::StringOutputStream protobufOutput(&protobufString);
                        auto protobufWriter = NYT::NYson::CreateProtobufWriter(&protobufOutput, NYson::ReflectProtobufMessageType(field.MutableMessage()->GetDescriptor()));
                        NYson::ParseYsonStringBuffer({val.Data.String, val.Length}, NYson::EYsonType::Node, protobufWriter.get());
                        Y_PROTOBUF_SUPPRESS_NODISCARD field.MutableMessage()->ParseFromArray(protobufString.data(), protobufString.length());
                    } else {
                        Y_PROTOBUF_SUPPRESS_NODISCARD field.MutableMessage()->ParseFromArray(val.Data.String, val.Length);
                    }
                } else if (field.IsString()) {
                    auto dbg = NYson::TYsonString(val.AsString());
                    field.Set(NYT::NYson::ConvertToYsonString(dbg, NYson::EYsonFormat::Text).ToString());
                } else if (const auto enumDesc = desc->enum_type()) {
                    if (const auto enumVal = enumDesc->FindValueByName(val.AsString())) {
                        field.Set(enumVal);
                    } else {
                        THROW_ERROR_EXCEPTION("Unknown value '%v' for enum %v", val.AsString(), enumDesc->full_name());
                    }
                } else {
                    THROW_ERROR_EXCEPTION("Unknown value type stored in string");
                }
                continue;
            case EValueType::Min:
            case EValueType::Max:
            case EValueType::TheBottom:
                THROW_ERROR_EXCEPTION("Bad value type: %v", val.Type);
        }
        THROW_ERROR_EXCEPTION("Not an EValueType: %v", val.Type);
    }

    // Write result
    msg->GetReflection()->Swap(msg, parsed.Get());

} catch (std::exception const& ex) {
    THROW_ERROR_EXCEPTION("Error while parsing unversioned row")
        << ex
        << TErrorAttribute("protobuf", ReprStr())
        << TErrorAttribute("schema", Schema())
        << TErrorAttribute("row", row);
}

TUnversionedRow TProtoSchema::MessageToRow(
    const NProtoBuf::Message* msg,
    TRowBufferPtr buffer,
    TConvertRowOptions const& opts) const
{
    TUnversionedRowBuilder builder;

    auto refl = msg->GetReflection();

    int pendingFields = 0;
    if (opts.Strict) {
        TVector<TFieldDescriptorPtr> fields;
        refl->ListFields(*msg, &fields);
        pendingFields = fields.size();
    }

    for (size_t id : xrange(IdToField.size())) {
        TFieldDescriptorPtr desc = IdToField[id];
        if (!desc) {
            // Skipped system column
            continue;
        }

        NYT::NTableClient::EValueFlags flags = NYT::NTableClient::EValueFlags::None;
        if (!refl->HasField(*msg, desc) && GetDeltaField(desc)) {
            desc = GetDeltaField(desc);
            flags |= NYT::NTableClient::EValueFlags::Aggregate;
        }

        NProtoBuf::TConstField field{*msg, desc};
        if (!field.HasValue()) {
            if (!opts.Partial) {
                builder.AddValue(MakeUnversionedSentinelValue(EValueType::Null, id));
            }
            continue;
        }

        using namespace NTableClient;
        switch (desc->type()) {
            case EFieldType::TYPE_INT64:
            case EFieldType::TYPE_INT32:
            case EFieldType::TYPE_SFIXED32:
            case EFieldType::TYPE_SFIXED64:
            case EFieldType::TYPE_SINT32:
            case EFieldType::TYPE_SINT64:
                builder.AddValue(MakeUnversionedInt64Value(field.Get<i64>(), id, flags));
                break;
            case EFieldType::TYPE_ENUM:
                builder.AddValue(MakeUnversionedStringValue(field.Get<const NProtoBuf::EnumValueDescriptor*>()->name(), id, flags));
                break;
            case EFieldType::TYPE_UINT64:
            case EFieldType::TYPE_UINT32:
            case EFieldType::TYPE_FIXED32:
            case EFieldType::TYPE_FIXED64:
                builder.AddValue(MakeUnversionedUint64Value(field.Get<ui64>(), id, flags));
                break;
            case EFieldType::TYPE_DOUBLE:
            case EFieldType::TYPE_FLOAT:
                builder.AddValue(MakeUnversionedDoubleValue(field.Get<double>(), id, flags));
                break;
            case EFieldType::TYPE_BOOL:
                builder.AddValue(MakeUnversionedBooleanValue(field.Get<bool>(), id, flags));
                break;
            case EFieldType::TYPE_STRING:
            case EFieldType::TYPE_BYTES:
            {
                TUnversionedValue value;
                if (GetForcedTypeOrDefault(desc->options().GetRepeatedExtension(NYT::flags), ESimpleLogicalValueType::String) == ESimpleLogicalValueType::Any)
                {
                    value = MakeUnversionedAnyValue(field.Get<TString>(), id, flags);
                } else {
                    value = MakeUnversionedStringValue(field.Get<TString>(), id, flags);
                }
                if (!opts.Reference) {
                    value = buffer->CaptureValue(value);
                }
                builder.AddValue(value);
                break;
            }
            case EFieldType::TYPE_MESSAGE:
            {
                auto child = field.Get<NProtoBuf::Message>();
                size_t bytes = 0;
                char* data = NULL;
                if (desc->options().HasExtension(message_to_yson) &&
                    desc->options().GetExtension(message_to_yson))
                {
                    TString yson;
                    TStringOutput output(yson);
                    NYson::TYsonWriter writer(&output);
                    NYson::WriteProtobufMessage(&writer, *child);
                    bytes = yson.length();
                    data = buffer->GetPool()->AllocateUnaligned(bytes);
                    yson.copy(data, bytes);
                } else {
                    bytes = child->ByteSize();
                    data = buffer->GetPool()->AllocateUnaligned(bytes);
                    Y_PROTOBUF_SUPPRESS_NODISCARD child->SerializeToArray(data, bytes);
                }

                if (GetForcedTypeOrDefault(desc->options().GetRepeatedExtension(NYT::flags), ESimpleLogicalValueType::String) == ESimpleLogicalValueType::Any)
                {
                    builder.AddValue(MakeUnversionedAnyValue({data, bytes}, id, flags));
                } else {
                    builder.AddValue(MakeUnversionedStringValue({data, bytes}, id, flags));
                }
                break;
            }
            default:
                Y_FAIL("Unsupported protobuf field type: %s", field.Field()->type_name());
        }

        --pendingFields;
    }

    if (opts.Strict && pendingFields != 0) {
        THROW_ERROR_EXCEPTION("Some fields set doesn't meets schema (strict mode set)")
            << TErrorAttribute("schema", Schema())
            << TErrorAttribute("message", NProtoBuf::ShortUtf8DebugString(*msg));
    }

    return buffer->CaptureRow(builder.GetRow(), false /* deep */);
}

TUnversionedOwningRow TProtoSchema::MessageToRow(
    const NProtoBuf::Message* msg,
    TConvertRowOptions const& opts) const
{
    TRowBufferPtr buffer = New<TRowBuffer>();
    return TUnversionedOwningRow{MessageToRow(msg, buffer, opts)};
}

static bool HiddenInPrimarySchema(TFieldDescriptorPtr field) {
    TStringBuf name = GetColumnName(field);
    return name.StartsWith(SystemColumnNamePrefix) && !(name == TimestampColumnName);
}

static void VerifySystemColumnType(TFieldDescriptorPtr field) {
    using namespace NTableClient;
    TStringBuf name = GetColumnName(field);
    if (name == TabletIndexColumnName) {
        Y_ENSURE(field->type() == EFieldType::TYPE_INT64);
    } else if (name == RowIndexColumnName) {
        Y_ENSURE(field->type() == EFieldType::TYPE_INT64);
    } else if (name == TimestampColumnName) {
        Y_ENSURE(field->type() == EFieldType::TYPE_UINT64);
    }
}

static TColumnSchema DeduceColumnSchema(TFieldDescriptorPtr field) {
    Y_ENSURE(!field->is_repeated(), "Repeated fields are not supported");
    auto& options = field->options();
    ESimpleLogicalValueType columnType = GetForcedTypeOrDefault(options.GetRepeatedExtension(NYT::flags), MapFieldType(field->type()));
    TColumnSchema column(TString{GetColumnName(field)}, columnType);
    Y_ENSURE(column.GetWireType() != EValueType::TheBottom,
        Format("Bad field type: %v", field->type_name()));

    if (IsKeyColumn(field)) {
        column.SetSortOrder(ESortOrder::Ascending);
    }
    if (options.HasExtension(lock)) {
        column.SetLock(options.GetExtension(lock));
    }
    if (options.HasExtension(expression)) {
        column.SetExpression(options.GetExtension(expression));
    }
    if (options.HasExtension(group)) {
        column.SetGroup(options.GetExtension(group));
    }
    return column;
}

static EOptimizeFor MapOptimizeFor(NYT::ETableOptimizeFor type) {
    switch (type) {
    case NYT::OF_LOOKUP:
        return EOptimizeFor::Lookup;
    case NYT::OF_SCAN:
        return EOptimizeFor::Scan;
    }
}

void TProtoSchema::DeduceSchema(TDescriptorPtr desc) try {
    int nfields = desc->field_count();
    std::vector<TColumnSchema> columns;
    columns.reserve(nfields);
    NameToFieldTag.reserve(nfields);
    IdToField.reserve(nfields);

    if (desc->options().HasExtension(optimize_for)) {
        OptimizeFor_ = MapOptimizeFor(desc->options().GetExtension(optimize_for));
    }

    bool uniqueKeys = false;
    TFieldDescriptorPtr deltaField = nullptr;

    for (int i : xrange(nfields)) {
        TFieldDescriptorPtr field = desc->field(i);
        auto& options = field->options();
        VerifySystemColumnType(field);
        if (HiddenInPrimarySchema(field)) {
            continue;
        }

        if (options.HasExtension(aggregate)) {
            Y_ENSURE(!deltaField,
                "Set aggregate=true only for delta field");

            auto oneof = field->containing_oneof();
            Y_ENSURE(oneof && oneof->field_count() == 2,
                "Group your aggregate delta field into oneof with value field");

            if (oneof->field(0) == field) { // first field
                deltaField = field;
            } else { // second field
                Y_ENSURE(columns.size() > 0);
                columns.back().SetAggregate(options.GetExtension(aggregate));
            }
            continue;
        }


        TColumnSchema& column = columns.emplace_back();
        column = DeduceColumnSchema(field);
        if (deltaField) { // from previous step
            Y_ENSURE(deltaField->type() == field->type(),
                "Delta field must have exactly same type as column field");
            column.SetAggregate(deltaField->options().GetExtension(aggregate));
            deltaField = nullptr;
        }
        uniqueKeys |= static_cast<bool>(column.SortOrder());

        NameToFieldTag[column.Name()] = field->number();
        IdToField.push_back(field);
    }

    Schema_ = TTableSchema{
        std::move(columns),
        true, /* strict */
        uniqueKeys
    };
    ValidateTableSchema(Schema_, true /* dynamic */);
} catch (const std::exception& ex) {
    THROW_ERROR_EXCEPTION("Error deducing table schema")
        << ex
        << NYT::TErrorAttribute("protobuf", desc->full_name());
}

void TProtoSchema::ApplyKind(ETableSchemaKind kind) {
    switch (kind) {
    case ETableSchemaKind::Primary:
        return;
    case ETableSchemaKind::Lookup:
        MutateSchema(*Schema_.ToLookup());
        return;
    case ETableSchemaKind::Delete:
        MutateSchema(*Schema_.ToDelete());
        return;
    case ETableSchemaKind::Write:
        MutateSchema(*Schema_.ToWrite());
        return;
    case ETableSchemaKind::Query:
        MutateSchema(*Schema_.ToQuery());
        return;
    case ETableSchemaKind::VersionedWrite:
        Y_FAIL("JUPITER-169");
        return;
    default:
        Y_UNREACHABLE();
    }
    Y_FAIL();
}

void TProtoSchema::MutateSchema(TTableSchema sub, bool checkAttributes) {
    // Verify modification
    bool hasCommon = false; // true if we have some common columns
    size_t newSysColumnCount = 0; // number of sub sys columns not founded in primary schema
    for (auto& column : sub.Columns()) {
        TStringBuf name = column.Name();

        bool ok = true;
        if (auto orig = Schema_.FindColumn(name)) {
            using NYson::ConvertToYsonString;
            if (checkAttributes) {
                ok = *orig == column;
            } else {
                ok = orig->GetWireType() == column.GetWireType();
            }
            hasCommon = true;
        } else {
            if (name.StartsWith(SystemColumnNamePrefix)) {
                ++newSysColumnCount;
            }
        }

        if (!ok) {
            THROW_ERROR_EXCEPTION("New schema conflicts with primary schema")
                << TErrorAttribute("column", name)
                << TErrorAttribute("protobuf", Descriptor()->full_name())
                << TErrorAttribute("primary_schema", Schema_)
                << TErrorAttribute("target_schema", sub);
        }
    }

    if (!hasCommon && !sub.Columns().empty() && newSysColumnCount != sub.Columns().size()) {
        THROW_ERROR_EXCEPTION("New schema has no intersection with primary schema")
            << TErrorAttribute("protobuf", Descriptor()->full_name())
            << TErrorAttribute("primary_schema", Schema_)
            << TErrorAttribute("target_schema", sub);
    }

    Schema_ = std::move(sub);

    // Add mappings for system columns if needed
    for (int i : xrange(Desc_->field_count())) {
        auto field = Desc_->field(i);
        auto name = GetColumnName(field);
        if (!HiddenInPrimarySchema(field)) {
            continue;
        }
        if (auto column = Schema_.FindColumn(name)) {
            Y_ENSURE(DeduceColumnSchema(field).GetWireType() == column->GetWireType(),
                Format("Unexpected type of system column: %s", name, column->GetWireType()));
            NameToFieldTag[name] = field->number();
        }
    }

    // Remove NameToField fields
    TVector<TString> toRemove;
    for (auto& p : NameToFieldTag) {
        if (!Schema_.FindColumn(p.first)) {
            toRemove.push_back(p.first);
        }
    }
    for (auto& name : toRemove) {
        NameToFieldTag.erase(name);
    }

    // Build mappings
    IdToField.resize(Schema_.GetColumnCount());
    Fill(IdToField.begin(), IdToField.end(), nullptr);
    for (auto& column : Schema_.Columns()) {
        int tag = NameToFieldTag[column.Name()];
        IdToField[Schema_.GetColumnIndex(column)] = Desc_->FindFieldByNumber(tag);
    }
}

void TProtoSchema::Finish() {
    NameTable_ = TNameTable::FromSchema(Schema_);

    size_t ncolumns = Schema().GetColumnCount();
    YT_VERIFY(NameToFieldTag.size() == ncolumns);
    YT_VERIFY(IdToField.size() == ncolumns);

    for (size_t id : xrange(ncolumns)) {
        TStringBuf name = Schema_.Columns().at(id).Name();
        if (TFieldDescriptorPtr field = IdToField[id]) {
            YT_VERIFY(GetColumnName(field) == name);
            YT_VERIFY(Desc_->FindFieldByNumber(NameToFieldTag[name]) == field);
        }
    }
}

TProtoSchema::TProtoSchema(TDescriptorPtr desc, ETableSchemaKind kind)
    : Desc_(desc)
    , Kind_(kind)
{
    DeduceSchema(desc);
    ApplyKind(kind);
    Finish();
}

TProtoSchema::TProtoSchema(TDescriptorPtr desc, TTableSchema schema)
    : Desc_(desc)
    , Kind_(ETableSchemaKind::Primary)
{
    DeduceSchema(desc);
    MutateSchema(std::move(schema), false /* checkAttributes */);
    Finish();
}

TProtoSchema TProtoSchema::Filter(TColumnFilter filter) const {
    ConvertColumnFilter(filter);
    return TProtoSchema{Descriptor(), *Schema().Filter(filter)};
}

struct TSchemaCompare {
    using TKey = std::tuple<TDescriptorPtr, ETableSchemaKind>;

    TKey RefKey(TProtoSchema const& s) const noexcept {
        return TKey{s.Descriptor(), s.Kind()};
    }

    TKey RefKey(TKey t) const noexcept {
        return t;
    }

    template<class T, class U>
    int operator()(T const& lhs, U const& rhs) const noexcept {
        if (RefKey(lhs) < RefKey(rhs)) {
            return -1;
        } else if (RefKey(lhs) == RefKey(rhs)) {
            return 0;
        } else {
            return 1;
        }
    };
};

struct TSchemaCache {
    TMemoryPool Pool{10 * 1024 * 1024};
    ::NThreading::TSkipList<TProtoSchema, TSchemaCompare> Store{Pool};
    TSpinLock Lock;
};

TProtoSchemaPtr TProtoSchema::Get(TDescriptorPtr desc, ETableSchemaKind kind) {
    TSchemaCache* cache = Singleton<TSchemaCache>();
    auto& skiplist = cache->Store;
    auto key = std::make_tuple(desc, kind);
    auto compare = TSchemaCompare{};
    auto it = skiplist.SeekTo(key);

    if (Y_LIKELY(it.IsValid() && compare(it.GetValue(), key) == 0)) {
        // fast lock-free way
        return &it.GetValue();
    } else {
        TProtoSchema schema{desc, kind};
        {
            TGuard<TSpinLock> guard{cache->Lock};
            skiplist.Insert(std::move(schema));
        }
        return Get(desc, kind);
    }
}

} // NProtoApi
} // NYT
