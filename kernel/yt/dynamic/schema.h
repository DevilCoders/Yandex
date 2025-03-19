#pragma once

#include "fwd.h"

#include <yt/yt/client/tablet_client/table_mount_cache.h>
#include <yt/yt/client/table_client/schema.h>
#include <yt/yt/client/table_client/unversioned_row.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>


namespace NYT {
namespace NProtoApiTest { class TTestProtoSchema; }
namespace NProtoApi {

using NTabletClient::ETableSchemaKind;
using EOptimizeFor = NTableClient::EOptimizeFor;

struct TConvertRowOptions {
    /*
     * Don't overwrite undefined values as null.
     */
    bool Partial{false};

    /*
     * Don't copy string data.
     */
    bool Reference{false};

    /*
     * Fails if message contains value not in schema.
     */
    bool Strict{false};
};

class TProtoSchema {
public:
    /*
     * Constructs schema by protobuf description and kind.
     */
    explicit TProtoSchema(TDescriptorPtr desc, ETableSchemaKind kind);

    /*
     * Explicitly sets given schema and checks that it is convertible to message.
     */
    explicit TProtoSchema(TDescriptorPtr desc, TTableSchema schema);

    /*
     * Constructs schema by parts of protobuf
     */

    static TProtoSchemaPtr Get(TDescriptorPtr desc, ETableSchemaKind kind);

    template<class TProto>
    static TProtoSchemaPtr Get(ETableSchemaKind kind) {
        return Get(TProto::descriptor(), kind);
    }

    TDescriptorPtr Descriptor() const noexcept {
        return Desc_;
    }

    ETableSchemaKind Kind() const noexcept {
        return Kind_;
    }

    const TTableSchema& Schema() const noexcept {
        return Schema_;
    }

    EOptimizeFor OptimizeFor() const noexcept {
        return OptimizeFor_;
    }

    TNameTablePtr NameTable() const noexcept {
        return NameTable_;
    }

    /*
     * Like python repr().
     */
    TString ReprStr() const;

    /*
     * Returns field by name or nullptr.
     */
    TFieldDescriptorPtr FieldByColumnName(TStringBuf name) const;

    /*
     * Converts column filter from protobuf tag-based to column id-based.
     */
    void ConvertColumnFilter(TColumnFilter& filter) const;

    /*
     * Apply filter to protobuf columns (by tag numbers).
     */
    TProtoSchema Filter(TColumnFilter filter) const;

    /*
     * Converts unversioned value to protobuf message using this schema.
     * Fails on bad column id or type mismatch.
     */
    void RowToMessage(
        TUnversionedRow const& row,
        NProtoBuf::Message* msg,
        TConvertRowOptions const& opts = {}) const;

    /*
     * Converts protobuf message to unversioned row.
     */
    TUnversionedRow MessageToRow(
        const NProtoBuf::Message* msg,
        TRowBufferPtr buffer,
        TConvertRowOptions const& opts = {}) const;

    TUnversionedOwningRow MessageToRow(
        const NProtoBuf::Message* msg,
        TConvertRowOptions const& opts = {}) const;

protected:
    void DeduceSchema(TDescriptorPtr desc);
    void ApplyKind(ETableSchemaKind kind);
    void MutateSchema(TTableSchema sub, bool checkAttributes = true);
    void Finish();

private:
    friend class ::NYT::NProtoApiTest::TTestProtoSchema;

    TDescriptorPtr Desc_;
    ETableSchemaKind Kind_;
    EOptimizeFor OptimizeFor_ = EOptimizeFor::Lookup;
    TTableSchema Schema_;
    TNameTablePtr NameTable_;

    THashMap<TString, int> NameToFieldTag;
    TVector<TFieldDescriptorPtr> IdToField;
};

} // NProtoApi
} // NYT
