#pragma once

#include <yt/yt/client/api/public.h>
#include <yt/yt/client/scheduler/public.h>
#include <yt/yt/client/table_client/public.h>
#include <yt/yt/client/tablet_client/public.h>
#include <yt/yt/client/transaction_client/public.h>
#include <yt/yt/core/actions/public.h>
#include <yt/yt/core/concurrency/scheduler.h>
#include <yt/yt/core/logging/public.h>
#include <yt/yt/core/misc/public.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/common.h>

namespace NYT {

namespace NProtoApi {
    // Aliases
    using NTableClient::EValueType;
    using NTableClient::ESimpleLogicalValueType;
    using NTableClient::TColumnFilter;
    using NTableClient::TNameTable;
    using NTableClient::TNameTablePtr;
    using NTableClient::TColumnSchema;
    using NTableClient::TTableSchema;
    using NTableClient::TTableSchemaPtr;
    using TKey = NTableClient::TUnversionedRow;
    using TOwningKey = NTableClient::TUnversionedOwningRow;
    using NTableClient::TUnversionedOwningRow;
    using NTableClient::TUnversionedRow;
    using NTableClient::TUnversionedValue;
    using NTableClient::TUnversionedRowBuilder;
    using NTableClient::TUnversionedOwningRowBuilder;
    using NTableClient::TRowBuffer;
    using NTableClient::TRowBufferPtr;

    using NTabletClient::ETabletState;
    using NTabletClient::TTableMountInfoPtr;

    using NTransactionClient::TTimestamp;
    using NTransactionClient::MinTimestamp;
    using NTransactionClient::MaxTimestamp;
    using NTransactionClient::NullTimestamp;
    using NTransactionClient::SyncLastCommittedTimestamp;
    using NTransactionClient::AsyncLastCommittedTimestamp;
    using NTransactionClient::AllCommittedTimestamp;

    using NTransactionClient::ETransactionType;
    using NTransactionClient::EAtomicity;
    using NTransactionClient::EDurability;

    using NConcurrency::WaitFor;

    using TDescriptorPtr = const NProtoBuf::Descriptor*;
    using TFieldDescriptorPtr = const NProtoBuf::FieldDescriptor*;
    using EFieldType = NProtoBuf::FieldDescriptor::Type;

    // Logger
    extern NYT::NLogging::TLogger Logger;

    // schema.h
    class TProtoSchema;
    using TProtoSchemaPtr = const TProtoSchema*;

    // table.h
    class TTableBase;
    template<class TProto> class TTable;

    // proto_api.h
    struct TLookupRowsOptions;
    struct TSelectRowsOptions;
    struct TWriteRowsOptions;
    struct TDeleteRowsOptions;

    class TRowsetBuilderBase;
    template<class TProto> class TRowsetBuilder;
    template<class TProto> class TLookupRowsBuilder;
    template<class TProto> class TWriteRowsBuilder;
    template<class TProto> class TDeleteRowsBuilder;

    template<class TProto> struct TSelectRowsResult;
    class TRowsInput;
} // NProtoApi

} // NYT
