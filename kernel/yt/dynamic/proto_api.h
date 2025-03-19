#pragma once

#include "fwd.h"
#include "schema.h"

#include <kernel/yt/utils/yt_utils.h>

#include <yt/yt/core/actions/bind.h>
#include <yt/yt/core/logging/log.h>
#include <yt/yt/core/misc/range.h>
#include <yt/yt/client/api/client.h>
#include <yt/yt/client/api/rowset.h>
#include <yt/yt/client/api/transaction.h>
#include <yt/yt/client/query_client/query_statistics.h>
#include <yt/yt/client/tablet_client/table_mount_cache.h>

#include <library/cpp/yson/node/node_io.h>
#include <library/cpp/yt/assert/assert.h>

#include <util/generic/vector.h>
#include <util/generic/maybe.h>

#include <type_traits>
#include <iterator>

namespace NYT {
namespace NProtoApi {

///////////////////////////////////////////////////////////////////////////////

using NQueryClient::TQueryStatistics;
using NTabletClient::ETableSchemaKind;
using NApi::IUnversionedRowsetPtr;

struct TLookupRowsOptions : NApi::TLookupRowsOptions
{
};

struct TSelectRowsOptions : NApi::TSelectRowsOptions
{
};

struct TWriteRowsOptions : NApi::TModifyRowsOptions
{
};

struct TDeleteRowsOptions : NApi::TModifyRowsOptions
{
};

///////////////////////////////////////////////////////////////////////////////

class TRowsetBuilderBase {
public:
    void Reserve(size_t n) {
        Rows.reserve(n);
    }

    size_t Size() const noexcept {
        return Rows.size();
    }

    TProtoSchemaPtr GetSchema() const noexcept {
        return Schema;
    }

    /*
     * Move data to the range. Size() will return 0 after this.
     * Example: std::move(builder).Finish()
     */
    TSharedRange<TUnversionedRow> Finish() &&;

    /*
     * Copy data to the range. Size() will not change after this.
     */
    TSharedRange<TUnversionedRow> Finish() const&;

    /*
     * Clears row and string data.
     */
    void Clear();

protected:
    explicit TRowsetBuilderBase(TProtoSchemaPtr schema);
    virtual ~TRowsetBuilderBase() noexcept;

    virtual void DoAdd(const NProtoBuf::Message& msg, TConvertRowOptions const& opts) {
        Rows.push_back(Schema->MessageToRow(&msg, Buffer, opts));
    }

private:
    TProtoSchemaPtr Schema;
    TRowBufferPtr Buffer;
    TVector<TUnversionedRow> Rows;
};

template<class TProto>
class TRowsetBuilder : public TRowsetBuilderBase {
public:
    explicit TRowsetBuilder(ETableSchemaKind kind)
        : TRowsetBuilderBase(TProtoSchema::Get<TProto>(kind))
    {
    }

    TRowsetBuilder& Add(const TProto& msg, TConvertRowOptions const& opts = {}) & {
        DoAdd(msg, opts);
        return *this;
    }

    TRowsetBuilder&& Add(const TProto& msg, TConvertRowOptions const& opts = {}) && {
        DoAdd(msg, opts);
        return std::move(*this);
    }

    TRowsetBuilder& AddPartial(const TProto& msg) & {
        TConvertRowOptions opts;
        opts.Partial = true;
        DoAdd(msg, opts);
        return *this;
    }

    TRowsetBuilder&& AddPartial(const TProto& msg) && {
        TConvertRowOptions opts;
        opts.Partial = true;
        DoAdd(msg, opts);
        return std::move(*this);
    }

    TRowsetBuilder& AddRef(const TProto& msg) & {
        TConvertRowOptions opts;
        opts.Reference = true;
        DoAdd(msg, opts);
        return *this;
    }

    TRowsetBuilder&& AddRef(const TProto& msg) && {
        TConvertRowOptions opts;
        opts.Reference = true;
        DoAdd(msg, opts);
        return std::move(*this);
    }
};

template<class TProto>
class TLookupRowsBuilder : public TRowsetBuilder<TProto>
{
public:
    TLookupRowsBuilder()
        : TRowsetBuilder<TProto>{ETableSchemaKind::Lookup}
    {
    }
};

template<class TProto>
class TWriteRowsBuilder : public TRowsetBuilder<TProto>
{
public:
    TWriteRowsBuilder()
        : TRowsetBuilder<TProto>{ETableSchemaKind::Write}
    {
    }
};

template<class TProto>
class TDeleteRowsBuilder : public TRowsetBuilder<TProto>
{
public:
    TDeleteRowsBuilder()
        : TRowsetBuilder<TProto>{ETableSchemaKind::Delete}
    {
    }
};

///////////////////////////////////////////////////////////////////////////////

template<class TProto, class TOutputIterator>
void ReadRows(
    TProtoSchemaPtr schema,
    TRange<TUnversionedRow> rows,
    TOutputIterator out)
{
    TProto record;
    for (const TUnversionedRow& row : rows) {
        schema->RowToMessage(row, &record);
        *out = std::move(record);
        ++out;
    }
}

/*
 * Writes given rowset to container of protobufs using embedded schema.
 * Assumes that TUnversionedValue ids are relative to rowset schema.
 * Throws (without modifying out) if there are no field in protobuf for some column in rowset schema.
 */
template<class TProto, class TOutputIterator>
void ReadRows(IUnversionedRowsetPtr rowset, TOutputIterator out) {
    TProtoSchema schema{TProto::descriptor(), *rowset->GetSchema()};
    ReadRows<TProto>(&schema, rowset->GetRows(), out);
}

template<class TProto>
TVector<TProto> ReadRows(IUnversionedRowsetPtr rowset) {
    TVector<TProto> out;
    ReadRows<TProto>(std::move(rowset), std::back_inserter(out));
    return out;
}

/*
 * This variant use proto-derived schema to map value ids.
 */
template<class TProto>
TVector<TProto> ReadLookupRows(IUnversionedRowsetPtr rowset) {
    TProtoSchema schema{TProto::descriptor(), *rowset->GetSchema()};

    TVector<TProto> out;
    ReadRows<TProto>(&schema, rowset->GetRows(), std::back_inserter(out));
    return out;
}

template<class TProto>
struct TSelectRowsResult {
    TVector<TProto> Rowset;
    NQueryClient::TQueryStatistics Statistics;
};

template<class TProto>
TSelectRowsResult<TProto> ReadSelectedRows(NApi::TSelectRowsResult result) {
    return {
        ReadRows<TProto>(std::move(result.Rowset)),
        std::move(result.Statistics)
    };
}

///////////////////////////////////////////////////////////////////////////////

/*
 * Converts different types of input range representations.
 */


class TRowsInput {
public:
    TSharedRange<TUnversionedRow> Range;
    TProtoSchemaPtr Schema;

private:
    template<typename TProto>
    void FillMe(TRowsetBuilder<TProto>&& builder) {
        Schema = builder.GetSchema();
        Range = std::move(builder).Finish();
    }

public:
    template<typename TProto, typename TContainer, typename = std::enable_if_t<
        !std::is_same<TProto, TContainer>::value
     && !std::is_base_of<TRowsetBuilder<TProto>, TContainer>::value
    >>
    void Fill(const TContainer& rows, ETableSchemaKind kind) {
        TRowsetBuilder<TProto> builder{kind};
        for (const TProto& row : rows)
            builder.Add(row);
        FillMe<TProto>(std::move(builder));
    }

    template<typename TProto>
    void Fill(TRowsetBuilder<TProto>&& builder, ETableSchemaKind kind) {
        if (builder.GetSchema()->Descriptor() == TProto::descriptor()) {
            YT_VERIFY(kind == builder.GetSchema()->Kind());
        }
        Range = std::forward<TRowsetBuilder<TProto>>(builder).Finish();
        Schema = builder.GetSchema();
    }

    template<typename TProto>
    void Fill(const TProto& row, ETableSchemaKind kind) {
        TRowsetBuilder<TProto> builder{kind};
        builder.Add(row);
        FillMe<TProto>(std::move(builder));
    }

    template<typename TProto, typename TContainer, typename = std::enable_if_t<!std::is_same<TProto, TContainer>::value>>
    void FillPartial(const TContainer& rows, ETableSchemaKind kind) {
        TRowsetBuilder<TProto> builder{kind};
        for (const TProto& row : rows)
            builder.AddPartial(row);
        FillMe<TProto>(std::move(builder));
    }

    template<typename TProto>
    void FillPartial(const TProto& row, ETableSchemaKind kind) {
        TRowsetBuilder<TProto> builder{kind};
        builder.AddPartial(row);
        FillMe<TProto>(std::move(builder));
    }

public:
    TRowsInput() = default;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * This functions adapts YT dynamic tables API
 * transparently replacing all input/output arguments by protobuf representation.
 */

template<class TProto, class TKeys>
TFuture<TVector<TProto>> LookupRowsAsync(
    NApi::IClientBasePtr client,
    const NYPath::TYPath& path,
    TKeys && keys,
    TLookupRowsOptions opts = {})
{
    TRowsInput input;
    input.Fill<TProto>(std::forward<TKeys>(keys), ETableSchemaKind::Lookup);

    auto range = std::move(input.Range);
    auto names = input.Schema->NameTable();

    if (range.Size() > 0) {
        input.Schema->ConvertColumnFilter(opts.ColumnFilter);
        return std::move(client)
            ->LookupRows(path, names, range, opts)
             .Apply(BIND(&ReadLookupRows<TProto>));
    } else {
        return MakeFuture(TVector<TProto>{});
    }
}

/*
 * A syntastic sugar for single-row requests.
 */
template<class TProto>
TFuture<TMaybe<TProto>> LookupRowAsync(
    NApi::IClientBasePtr client,
    const NYPath::TYPath& path,
    TProto const& key,
    TLookupRowsOptions opts = {})
{
    return LookupRowsAsync<TProto>(std::move(client), path, key, opts)
        .Apply(BIND([](TVector<TProto> rows) -> TMaybe<TProto> {
            YT_VERIFY(rows.size() <= 1);
            if (rows.size() == 0) {
                return Nothing();
            } else {
                return std::move(rows[0]);
            }
        }));
}

template<class TProto, class TKeys>
TVector<TProto> LookupRows(
    NApi::IClientBasePtr client,
    const NYPath::TYPath& path,
    TKeys && keys,
    TLookupRowsOptions const& opts = {})
{
    auto lookup = LookupRowsAsync<TProto>(
        std::move(client), path, std::forward<TKeys>(keys), opts);
    return std::move(WaitFor(lookup).ValueOrThrow());
}

template<class TProto>
TMaybe<TProto> LookupRow(
    NApi::IClientBasePtr client,
    const NYPath::TYPath& path,
    TProto const& key,
    TLookupRowsOptions const& opts = {})
{
    auto lookup = LookupRowAsync<TProto>(std::move(client), path, key, opts);
    return std::move(WaitFor(lookup).ValueOrThrow());
}

template<class TProto, class TKeys>
TFuture<TVector<TProto>> LookupRowsVia(
    IInvokerPtr invoker,
    NApi::IClientBasePtr client,
    NYPath::TYPath const& path,
    TKeys && keys,
    TLookupRowsOptions opts = {})
{
    TRowsInput input;
    input.Fill<TProto>(std::forward<TKeys>(keys), ETableSchemaKind::Lookup);

    auto range = std::move(input.Range);
    auto names = input.Schema->NameTable();

    if (range.Size() > 0) {
        input.Schema->ConvertColumnFilter(opts.ColumnFilter);
        return BIND(&NApi::IClientBase::LookupRows, std::move(client))
            .AsyncVia(invoker)
            .Run(path, names, range, std::move(opts))
            .Apply(BIND(&ReadLookupRows<TProto>).AsyncVia(invoker));
    } else {
        return MakeFuture(TVector<TProto>{});
    }
}

template<class TProto>
TFuture<TMaybe<TProto>> LookupRowVia(
    IInvokerPtr invoker,
    NApi::IClientBasePtr client,
    NYPath::TYPath const& path,
    TProto const& key,
    TLookupRowsOptions const& opts = {})
{
    return BIND(&LookupRow<TProto>)
        .AsyncVia(std::move(invoker))
        .Run(client, path, key, opts);
}

///////////////////////////////////////////////////////////////////////////////

template<class TProto>
TFuture<TSelectRowsResult<TProto>> SelectRowsAsync(
    NApi::IClientBasePtr client,
    TString const& query,
    TSelectRowsOptions opts = {})
{
    return std::move(client)
        ->SelectRows(query, opts)
         .Apply(BIND(&ReadSelectedRows<TProto>));
}

template<class TProto>
TSelectRowsResult<TProto> SelectRows(
    NApi::IClientBasePtr client,
    TString const& query,
    TSelectRowsOptions opts = {})
{
    auto select = SelectRowsAsync<TProto>(std::move(client), query, opts);
    return std::move(WaitFor(select).ValueOrThrow());
}

template<class TProto>
TFuture<TSelectRowsResult<TProto>> SelectRowsVia(
    IInvokerPtr invoker,
    NApi::IClientBasePtr client,
    TString query,
    TSelectRowsOptions opts = {})
{
    return BIND(&SelectRows<TProto>)
        .AsyncVia(std::move(invoker))
        .Run(std::move(client), std::move(query), std::move(opts));
}

///////////////////////////////////////////////////////////////////////////////

template<class TProto, class TValues>
void WriteRows(
    NApi::ITransactionPtr tx,
    const NYPath::TYPath& path,
    TValues && values,
    const TWriteRowsOptions& opts = {})
{
    TRowsInput input;
    input.Fill<TProto>(std::forward<TValues>(values), ETableSchemaKind::Write);

    tx->WriteRows(path, input.Schema->NameTable(), std::move(input.Range), opts);
}


template<class TProto, class TValues>
void OverlayRows(
    NApi::ITransactionPtr tx,
    const NYPath::TYPath& path,
    TValues && values,
    const TWriteRowsOptions& opts = {})
{
    TRowsInput input;
    input.FillPartial<TProto>(std::forward<TValues>(values), ETableSchemaKind::Write);

    tx->WriteRows(path, input.Schema->NameTable(), std::move(input.Range), opts);
}


template<class TProto, class TKeys>
void DeleteRows(
    NApi::ITransactionPtr tx,
    const NYPath::TYPath& path,
    TKeys && keys,
    const TDeleteRowsOptions& opts = {})
{
    TRowsInput input;
    input.Fill<TProto>(std::forward<TKeys>(keys), ETableSchemaKind::Delete);

    tx->DeleteRows(path, input.Schema->NameTable(), std::move(input.Range), opts);
}

} // NProtoApi

/*
 * Simple shortcuts for dynamic tables API.
 * Note that this functions can be used without `NYT::` namespace prefix, thanks to ADL.
 */

inline TFuture<NApi::TTransactionCommitResult> CommitAsync(
    NApi::ITransactionPtr tx,
    NApi::TTransactionCommitOptions const& opts = {})
{
    return std::move(tx)->Commit(opts);
}

inline NApi::TTransactionCommitResult Commit(
    NApi::ITransactionPtr tx,
    NApi::TTransactionCommitOptions const& opts = {})
{
    using namespace NProtoApi;
    return std::move(WaitFor(CommitAsync(tx, opts)).ValueOrThrow());
}

inline TFuture<NApi::ITransactionPtr> StartTransactionAsync(
    NApi::IClientBasePtr client,
    NProtoApi::ETransactionType type = NProtoApi::ETransactionType::Tablet,
    NApi::TTransactionStartOptions const& opts = {})
{
    using namespace NProtoApi;
    return client->StartTransaction(type, opts);
}

inline NApi::ITransactionPtr StartTransaction(
    NApi::IClientBasePtr client,
    NProtoApi::ETransactionType type = NProtoApi::ETransactionType::Tablet,
    NApi::TTransactionStartOptions const& opts = {})
{
    using namespace NProtoApi;
    auto start = StartTransactionAsync(client, type, opts);
    return std::move(WaitFor(start).ValueOrThrow());
}


template<typename T>
T GetYtAttr(NApi::IClientBasePtr client, const TString& path, const TString& attrName) {
    auto fullPath = NJupiter::JoinYtMeta(path, attrName);
    return NYT::NYTree::ConvertTo<T>(NYT::NConcurrency::WaitFor(client->GetNode(fullPath)).ValueOrThrow());
}

template <>
inline TNode GetYtAttr(NApi::IClientBasePtr client, const TString& path, const TString& attrName) {
    auto fullPath = NJupiter::JoinYtMeta(path, attrName);
    auto ysonString = NYT::NConcurrency::WaitFor(client->GetNode(fullPath)).ValueOrThrow();
    Y_ENSURE(ysonString.GetType() == NYT::NYson::EYsonType::Node);
    return NYT::NodeFromYsonString(ysonString.AsStringBuf(), ::NYson::EYsonType::Node);
}


template<typename T>
void SetYtAttr(NApi::IClientBasePtr client, const TString& path, const TString& attrName, const T& value) {
    auto fullPath = path + "/@" + attrName;
    NYT::NConcurrency::WaitFor(client->SetNode(fullPath, NYT::NYson::ConvertToYsonString(value))).ThrowOnError();
}


template<typename T>
TMaybe<T> GetYtAttrMaybe(NApi::IClientBasePtr client, const TString& path, const TString& attrName) {
    if (NYT::NConcurrency::WaitFor(client->NodeExists(NJupiter::JoinYtMeta(path, attrName))).ValueOrThrow()) {
        return GetYtAttr<T>(client, path, attrName);
    } else {
        return Nothing();
    }
}

void WaitAndLock(
    NYT::NApi::ITransactionPtr transaction,
    const TString& nodePath,
    NYT::NCypressClient::ELockMode lockMode,
    TDuration timeout,
    NYT::NApi::TLockNodeOptions lockOptions = NYT::NApi::TLockNodeOptions());

} // NYT
