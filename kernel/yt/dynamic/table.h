#pragma once

#include "fwd.h"
#include "proto_api.h"
#include "schema.h"
#include "timestamp.h"

#include <yt/yt/client/ypath/public.h>

#include <util/generic/variant.h>
#include <util/generic/xrange.h>

namespace NYT {
namespace NProtoApi {

struct TTableOptions {
    /*
     * Checks that protobuf fields:
     * 1. Has at least one common column with table schema;
     * 2. Matches schema by type;
     */
    bool CheckSchema{true};

    /*
     * Check that table schema exatly matches protobuf description (with filter).
     */
    bool CheckSchemaStrictly{false};
};

class TTableBase {
public:
    using TPivotKeys = std::vector<TOwningKey>;

    /*
     * Creates table wrapper object. Semantics depends on options.
     */
    explicit TTableBase(
        NYPath::TYPath path,
        TDescriptorPtr desc,
        NApi::IClientPtr client = nullptr,
        TTableOptions const& opts = TTableOptions());

    /*
     * Creates table wrapper from non-null transaction.
     */
    explicit TTableBase(
        NYPath::TYPath path,
        TDescriptorPtr desc,
        NApi::ITransactionPtr transaction,
        TTableOptions const& opts = TTableOptions());

    /*
     * Creates untyped table wrapper.
     */
    explicit TTableBase(
        NYPath::TYPath path,
        NApi::IClientPtr client = nullptr);

    /*
     * Null state. All other methods will fail.
     */
    TTableBase() noexcept;
    void Reset();

    /*
     * Retrieve installed schema from attributes.
     */
    TTableSchema GetSchema() const;

    /*
     * Retrieve aggregated state of all tablets or ETabletState::Mixed
     */
    ETabletState GetState() const;

    /*
     * Waits for GetState() == state.
     */
    void WaitForState(ETabletState state) const;

    /*
     * Check if node exists.
     */
    bool Exists() const;

    /*
     * Creates dynamic table with TProto schema. overrides dynamic and schema
     * attributes with true and org schema
     */
    void Create(NYT::NYTree::IMapNodePtr attrsMap = nullptr) const;
    void Create(NYT::NYTree::IAttributeDictionaryPtr attrsMap) const;

    /*
     * Creates replicated table with TProto schema, overrides dynamic and schema
     * attributes with true and org schema
     *
     * NB: those methods don't create table replicas meta info and don't
     * create replica's tables
     */

    void CreateReplicatedTable(NYT::NYTree::IMapNodePtr attrsMap = nullptr) const;
    void CreateReplicatedTable(NYT::NYTree::IAttributeDictionaryPtr attrsMap) const;

    /*
     * Creates table replica's meta
     */
    NCypressClient::TNodeId CreateTableReplica(const TString& cluster, bool isSynchronized = false, TString replicaPath = "") const;

    /*
     * Removes table.
     */
    void Remove() const;

    /*
     * Checks if table schema meets protobuf (see options).
     */
    bool CheckSchema(bool strict) const;

    /*
     * Alters table type to dynamic with TProto schema.
     */
    void Alter() const;

    /*
     * Tablet management utilities.
     * Async-suffixed versions of doesn't wait for all tablets.
     */
    void Mount() const;
    void MountAsync() const;
    void Unmount() const;
    void UnmountAsync() const;

    /*
     * Just useful wrapper, create table if it doesn't exist
     * or check schema if table exists already and mount it.
     */
    void Setup(bool checkSchemaStrict = true) const;

    /* See https://wiki.yandex-team.ru/yt/userdoc/dynamictablesmapreduce/ */
    void Freeze() const;
    void FreezeAsync() const;
    void Unfreeze() const;
    void UnfreezeAsync() const;

    /*
     * Reshards ordered table or sorted table by hash column.
     */
    void Reshard(ui32 ntablets) const;

    /*
     * Reshards sorted table by explicit key ranges.
     * Not recommended, better to do this from python.
     */
    void Reshard(TPivotKeys const& pivotKeys) const;

    /*
     * Returns number of tablets. Faster than PivotKeys().size().
     */
    ui32 TabletCount() const;

    /*
     * Get (possibly cached) vector of pivot keys.
     * Holds: PivotKeys().size() == TabletCount()
     */
    TPivotKeys PivotKeys() const;

    /*
     * Computes tablet index by key.
     * 0 <= retval < TabletCount()
     */
    i32 TabletIndex(TUnversionedRow row) const;

    /*
     * Unowning sets transaction for read/write operations.
     * This state can be dropped by SetTransaction(nullptr) call.
     */
    void SetTransaction(NApi::ITransactionPtr transaction);

    /*
     * Sets client for this table.
     */
    void SetClient(NApi::IClientPtr client);

    /*
     * Returns transaction or nullptr.
     */
    NApi::ITransactionPtr GetTransaction() const;

    /*
     * Return transaction (if available) or client.
     */
    NApi::IClientBasePtr GetClientBase() const;

    const NYPath::TYPath& GetPath() const noexcept {
        return Path;
    }

    NApi::IClientPtr GetClient() const {
        return Client;
    }

    template<typename T>
    T GetAttribute(TString const& attributeName) const {
        return GetYtAttr<T>(GetClientBase(), GetPath(), attributeName);
    }

    TTimestamp GetTimestamp(ETimestampAttribute attribute) const;

private:
    NCypressClient::TNodeId Create(NYT::NObjectClient::EObjectType type,  NYT::NYTree::IAttributeDictionaryPtr attrsMap) const;
protected:
    NYPath::TYPath Path;
    TProtoSchemaPtr Schema;
    NApi::IClientPtr Client;
    TWeakPtr<NApi::ITransaction> Transaction;

    void Init(TTableOptions const& opts);
    std::vector<NYTree::INodePtr> GetTablets() const;
    TString PutThisIntoQuery(TString const& query) const;
};

template<class TProto>
class TTable : public TTableBase {
public:
    using TRowProto = TProto;

    TTable() noexcept = default;

    /*
     * Protobuf-centric versions of base functions.
     */
    TTable(NYPath::TYPath path,
           NApi::IClientPtr client = nullptr,
           TTableOptions const& opts = {})
        : TTableBase(
            std::move(path),
            TProto::descriptor(),
            std::move(client),
            opts
          )
    {
    }

    explicit TTable(
            NYPath::TYPath path,
            NApi::ITransactionPtr transaction,
            TTableOptions const& opts = {})
        : TTableBase(
            std::move(path),
            TProto::descriptor(),
            std::move(transaction),
            opts
          )
    {
    }

    void Reshard(ui32 ntablets) const {
        TTableBase::Reshard(ntablets);
    }

    void Reshard(std::vector<TProto> const& pivotKeys) const {
        TPivotKeys rawKeys{pivotKeys.size()};
        TConvertRowOptions opts;
        opts.Reference = true;
        opts.Partial = true;
        for (size_t i : xrange(pivotKeys.size())) {
            rawKeys[i] = Schema->MessageToRow(&pivotKeys[i], opts);
        }
        TTableBase::Reshard(rawKeys);
    }

    TVector<TProto> PivotKeys() const {
        auto pivotKeys = TTableBase::PivotKeys();
        TVector<TProto> protoKeys;
        protoKeys.resize(pivotKeys.size());
        for (size_t i : xrange(pivotKeys.size())) {
            Schema->RowToMessage(pivotKeys[i], &protoKeys[i]);
        }
        return protoKeys;
    }

    ui32 TabletIndex(TProto const& row) const {
        TConvertRowOptions opts;
        opts.Reference = true;
        return TTableBase::TabletIndex(Schema->MessageToRow(&row, opts));
    }

    [[nodiscard]] TTable In(NApi::ITransactionPtr tx) const {
        auto retval = TTable(*this);
        retval.SetTransaction(tx);
        return retval;
    }

    [[nodiscard]] TTable At(NApi::IClientPtr client) const {
        auto retval = TTable(*this);
        retval.SetClient(client);
        return retval;
    }

    /*
     * Shortcuts for dynamic table operations.
     */
    template<class TKeys>
    TFuture<TVector<TProto>> LookupRowsAsync(
        TKeys&& keys, TLookupRowsOptions opts = {}) const
    {
        auto client = GetClientBase();
        return NYT::NProtoApi::LookupRowsAsync<TProto>(
            std::move(client), Path, std::forward<TKeys>(keys), std::move(opts));
    }

    template<class TKeys>
    TVector<TProto> LookupRows(
        TKeys&& keys, TLookupRowsOptions opts = {}) const
    {
        auto lookup = LookupRowsAsync(std::forward<TKeys>(keys), std::move(opts));
        return std::move(WaitFor(lookup).ValueOrThrow());
    }

    template<class TKeys>
    TFuture<TVector<TProto>> LookupRowsVia(
        IInvokerPtr invoker, TKeys&& keys, TLookupRowsOptions opts = {}) const
    {
        auto client = GetClientBase();
        return NYT::NProtoApi::LookupRowsVia<TProto>(std::move(invoker),
            std::move(client), Path, std::forward<TKeys>(keys), std::move(opts));
    }

    TFuture<TMaybe<TProto>> LookupRowAsync(
        TProto const& key, TLookupRowsOptions opts = {}) const
    {
        auto client = GetClientBase();
        return NYT::NProtoApi::LookupRowAsync<TProto>(
            std::move(client), Path, key, std::move(opts));
    }

    TMaybe<TProto> LookupRow(TProto const& key, TLookupRowsOptions opts = {}) const
    {
        return std::move(WaitFor(LookupRowAsync(key, std::move(opts))).ValueOrThrow());
    }

    TFuture<TMaybe<TProto>> LookupRowVia(
        IInvokerPtr invoker, TProto key, TLookupRowsOptions opts = {}) const
    {
        auto client = GetClientBase();
        return NYT::NProtoApi::LookupRowVia<TProto>(std::move(invoker),
            std::move(client), Path, std::move(key), std::move(opts));
    }

    template<class TOut = TProto>
    TFuture<TSelectRowsResult<TOut>> SelectRowsAsync(
        TString const& query, TSelectRowsOptions opts = {}) const
    {
        auto client = GetClientBase();
        return NYT::NProtoApi::SelectRowsAsync<TOut>(
            std::move(client), PutThisIntoQuery(query), std::move(opts));
    }

    template<class TOut = TProto>
    TSelectRowsResult<TOut> SelectRows(
        TString const& query, TSelectRowsOptions opts = {}) const
    {
        auto select = SelectRowsAsync<TOut>(query, std::move(opts));
        return std::move(WaitFor(select).ValueOrThrow());
    }

    template<class TOut = TProto>
    TFuture<TSelectRowsResult<TOut>> SelectRowsVia(
        IInvokerPtr invoker, TString const& query, TSelectRowsOptions opts = {}) const
    {
        auto client = GetClientBase();
        return NYT::NProtoApi::SelectRowsVia<TOut>(
            std::move(invoker), std::move(client), PutThisIntoQuery(query), opts);
    }

    template<class TValues>
    void WriteRows(TValues&& values, TWriteRowsOptions const& opts = {}) const
    {
        NApi::ITransactionPtr tx = GetTransaction();
        bool commitImmediately = false;

        if (!tx) {
            tx = StartTransaction(Client);
            commitImmediately = true;
        }
        NYT::NProtoApi::WriteRows<TProto>(tx, Path, std::forward<TValues>(values), opts);
        if (commitImmediately) {
            WaitFor(tx->Commit()).ThrowOnError();
        }
    }

    template<class TValues>
    void OverlayRows(TValues&& values, TWriteRowsOptions const& opts = {}) const
    {
        NApi::ITransactionPtr tx = GetTransaction();
        bool commitImmediately = false;

        if (!tx) {
            tx = StartTransaction(Client);
            commitImmediately = true;
        }
        NYT::NProtoApi::OverlayRows<TProto>(tx, Path, std::forward<TValues>(values), opts);
        if (commitImmediately) {
            WaitFor(tx->Commit()).ThrowOnError();
        }
    }

    template<class TKeys>
    void DeleteRows(TKeys&& keys, TDeleteRowsOptions const& opts = {}) const
    {
        NApi::ITransactionPtr tx = GetTransaction();
        bool commitImmediately = false;

        if (!tx) {
            tx = StartTransaction(Client);
            commitImmediately = true;
        }
        NYT::NProtoApi::DeleteRows<TProto>(tx, Path, std::forward<TKeys>(keys), opts);
        if (commitImmediately) {
            WaitFor(tx->Commit()).ThrowOnError();
        }
    }
};

inline TString ToString(TTableBase const& table) {
    return table.GetPath();
}

template<class TProto>
TString ToString(TTable<TProto> const& table) {
    return table.GetPath();
}

} // NProtoApi
} // NYT
