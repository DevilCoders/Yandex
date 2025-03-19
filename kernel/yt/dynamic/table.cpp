#include "table.h"
#include "schema.h"

#include <yt/yt/client/tablet_client/table_mount_cache.h>
#include <yt/yt/core/misc/pattern_formatter.h>
#include <yt/yt/core/ytree/convert.h>
#include <yt/yt/core/logging/log.h>

#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>
#include <util/generic/ylimits.h>
#include <util/string/cast.h>

namespace NYT {
namespace NProtoApi {

TTableBase::TTableBase(NYPath::TYPath path, NApi::IClientPtr client)
    : Path(std::move(path))
    , Schema(nullptr)
    , Client(std::move(client))
{
    Init({});
}

TTableBase::TTableBase(
        NYPath::TYPath path,
        TDescriptorPtr desc,
        NApi::IClientPtr client,
        TTableOptions const& opts)
    : Path(std::move(path))
    , Schema(TProtoSchema::Get(desc, ETableSchemaKind::Primary))
    , Client(client)
{
    Init(opts);
}

TTableBase::TTableBase(
        NYPath::TYPath path,
        TDescriptorPtr desc,
        NApi::ITransactionPtr transaction,
        TTableOptions const& opts)
    : Path(std::move(path))
    , Schema(TProtoSchema::Get(desc, ETableSchemaKind::Primary))
    , Client(transaction->GetClient())
    , Transaction(MakeWeak(transaction))
{
    Init(opts);
}

TTableBase::TTableBase() noexcept
{
}

void TTableBase::Reset() {
    Path.clear();
    Schema = nullptr;
    Client.Reset();
    Transaction.Reset();
}

void TTableBase::Init(TTableOptions const& opts) {
    if (opts.CheckSchema | opts.CheckSchemaStrictly) {
        if (!Client || !Schema || !Exists()) {
            return;
        }

        if (!CheckSchema(opts.CheckSchemaStrictly)) {
            THROW_ERROR_EXCEPTION("Table schema check failed for %v", Path)
                << TErrorAttribute("schema", GetSchema())
                << TErrorAttribute("want", Schema->Schema())
                << TErrorAttribute("strict_check", opts.CheckSchemaStrictly);
        }
    }
}

TTableSchema TTableBase::GetSchema() const {
    NApi::TGetNodeOptions opts;
    NYson::TYsonString resp = WaitFor(Client->GetNode(Path + "/@schema", opts))
        .ValueOrThrow();
    return NYTree::ConvertTo<TTableSchema>(resp);
}

std::vector<NYTree::INodePtr> TTableBase::GetTablets() const {
    const NYT::NYson::TYsonString yson = Client->GetNode(Path + "/@tablets").Get().ValueOrThrow();
    return NYT::NYTree::ConvertToNode(yson)->AsList()->GetChildren();
}

ETabletState TTableBase::GetState() const {
    NYson::TYsonString yson = WaitFor(Client->GetNode(Path + "/@tablet_state"))
        .ValueOrThrow();
    return NYT::ParseEnum<ETabletState>(NYTree::ConvertTo<TString>(yson));
}

void TTableBase::WaitForState(ETabletState state) const {
    while (GetState() != state) {
        YT_LOG_INFO("Waiting for @tablet_state = %v (Path: %Qv)", state, GetPath());
        Sleep(TDuration::Seconds(1));
    }
}

bool TTableBase::Exists() const {
    NApi::TNodeExistsOptions opts;
    return WaitFor(Client->NodeExists(Path, opts))
        .ValueOrThrow();
}

NCypressClient::TNodeId TTableBase::Create(NYT::NObjectClient::EObjectType type,  NYT::NYTree::IAttributeDictionaryPtr attrsMap) const {
    NApi::TCreateNodeOptions opts;
    opts.Recursive = true;
    opts.IgnoreExisting = true;

    attrsMap->Set("dynamic", true);
    attrsMap->Set("schema", Schema->Schema());
    attrsMap->Set("optimize_for", Schema->OptimizeFor());
    opts.Attributes = std::move(attrsMap);

    return WaitFor(Client->CreateNode(Path, type, opts)).ValueOrThrow();
}

void TTableBase::Create(NYT::NYTree::IAttributeDictionaryPtr attrsMap) const {
    YT_LOG_INFO("Creating table node %Qv", GetPath());
    Create(NObjectClient::EObjectType::Table, attrsMap);
}

void TTableBase::Create(NYT::NYTree::IMapNodePtr attrsMap) const {
    auto attrs = NYT::NYTree::CreateEphemeralAttributes();
    if (attrsMap)
        attrs->MergeFrom(attrsMap);
    Create(std::move(attrs));
}

void TTableBase::CreateReplicatedTable(NYT::NYTree::IAttributeDictionaryPtr attrsMap) const {
    YT_LOG_INFO("Creating replicated table node %Qv", GetPath());
    Create(NObjectClient::EObjectType::ReplicatedTable, attrsMap);
}

void TTableBase::CreateReplicatedTable(NYT::NYTree::IMapNodePtr attrsMap) const {
    auto attrs = NYT::NYTree::CreateEphemeralAttributes();
    if (attrsMap)
        attrs->MergeFrom(attrsMap);
    CreateReplicatedTable(std::move(attrs));
}


NCypressClient::TNodeId TTableBase::CreateTableReplica(const TString& cluster, bool isSynchronized, TString replicaPath) const {
    TString mode = isSynchronized ? "sync" : "async";

    auto attrs = NYT::NYTree::CreateEphemeralAttributes();
    attrs->Set("table_path", GetPath());
    attrs->Set("cluster_name", cluster);
    attrs->Set("replica_path", replicaPath.empty() ? GetPath() : replicaPath);
    attrs->Set("mode", mode);

    NYT::NApi::TCreateObjectOptions opts;
    opts.Attributes = std::move(attrs);

    return WaitFor(Client->CreateObject(NObjectClient::EObjectType::TableReplica, opts)).ValueOrThrow();

}

void TTableBase::Remove() const {
    WaitFor(Client->RemoveNode(Path)).ThrowOnError();
}

bool TTableBase::CheckSchema(bool strict) const {
    Y_VERIFY(Schema);

    TTableSchema installed = GetSchema();
    TTableSchema want = Schema->Schema();

    if (strict) {
        return installed == want;
    }

    ui32 common = 0;
    for (TColumnSchema const& column : want.Columns()) {
        if (auto other = installed.FindColumn(column.Name())) {
            if (other->GetWireType() != column.GetWireType()) {
                return false;
            }
            ++common;
        }
    }

    return common > 0;
}

void TTableBase::Alter() const {
    Y_VERIFY(Schema);

    NApi::TAlterTableOptions opts;
    opts.Dynamic = true;
    opts.Schema = Schema->Schema();
    WaitFor(Client->AlterTable(Path, opts))
        .ThrowOnError();
}

void TTableBase::Mount() const {
    YT_LOG_INFO("Mounting %Qv", GetPath());
    MountAsync();
    WaitForState(ETabletState::Mounted);
}

void TTableBase::MountAsync() const {
    auto state = GetState();
    if (state == ETabletState::Mounted) {
        YT_LOG_INFO("Table %Qv is already mounted", GetPath());
        return;
    }
    if (state == ETabletState::Mounting) {
        YT_LOG_INFO("Table %Qv is in \"mounting\" state", GetPath());
        return;
    }
    WaitFor(Client->MountTable(Path))
        .ThrowOnError();
}

void TTableBase::Unmount() const {
    YT_LOG_INFO("Unmounting %Qv", GetPath());
    UnmountAsync();
    WaitForState(ETabletState::Unmounted);
}

void TTableBase::UnmountAsync() const {
    WaitFor(Client->UnmountTable(Path))
        .ThrowOnError();
}

void TTableBase::Setup(bool checkSchemaStrict) const {
    if (!Exists()) {
        Create();
    } else {
        CheckSchema(checkSchemaStrict);
    }
    Mount();
}

void TTableBase::Freeze() const {
    YT_LOG_INFO("Freezing %Qv", GetPath());
    FreezeAsync();
    WaitForState(ETabletState::Frozen);
}

void TTableBase::FreezeAsync() const {
    WaitFor(Client->FreezeTable(Path))
        .ThrowOnError();
}

void TTableBase::Unfreeze() const {
    YT_LOG_INFO("Unfreezing %Qv", GetPath());
    UnfreezeAsync();
    WaitForState(ETabletState::Mounted);
}

void TTableBase::UnfreezeAsync() const {
    WaitFor(Client->UnfreezeTable(Path))
        .ThrowOnError();
}

void TTableBase::Reshard(ui32 ntablets) const {
    Y_VERIFY(Schema);
    Y_VERIFY(ntablets > 0);

    auto& schema = Schema->Schema();
    if (schema.GetKeyColumnCount() == 0) {
        // ordered table
        YT_LOG_INFO("Sharding %Qv into %v tablets", Path, ntablets);
        WaitFor(Client->ReshardTable(Path, ntablets))
            .ThrowOnError();

        while (GetState() == ETabletState::Transient) {
            YT_LOG_INFO("Waiting for @tablet_state != transient of table %Qv", Path);
            Sleep(TDuration::Seconds(1));
        }
    } else {
        // sorted table
        EValueType type0 = schema.Columns()[0].GetWireType();
        if (!NTableClient::IsIntegralType(type0)) {
            THROW_ERROR_EXCEPTION(
                "Cannot autoshard table %v: first key column should have an integral type", Path);
        }

        TPivotKeys pivotKeys;
        pivotKeys.resize(ntablets);
        {
            TUnversionedOwningRowBuilder builder;
            pivotKeys[0] = builder.FinishRow();
        }
        for (size_t tablet : xrange(1u, ntablets)) {
            TUnversionedOwningRowBuilder builder;
            double border = static_cast<double>(Max<ui64>());
            border *= static_cast<double>(tablet) / ntablets;
            if (type0 == EValueType::Int64) {
                i64 key = static_cast<i64>(Min<i64>() + border);
                builder.AddValue(NTableClient::MakeUnversionedInt64Value(key));
            } else if (type0 == EValueType::Uint64) {
                ui64 key = static_cast<ui64>(border);
                builder.AddValue(NTableClient::MakeUnversionedUint64Value(key));
            } else {
                Y_FAIL();
            }
            pivotKeys[tablet] = builder.FinishRow();
        }
        Reshard(pivotKeys);
    }
}

void TTableBase::Reshard(TPivotKeys const& pivotKeys) const {
    Y_VERIFY(pivotKeys.size() > 0);
    YT_LOG_INFO("Sharding %Qv for %v tablets", GetPath(), pivotKeys.size());
    NApi::TReshardTableOptions opts;
    WaitFor(Client->ReshardTable(Path, pivotKeys, opts))
        .ThrowOnError();

        while (GetState() == ETabletState::Transient) {
            YT_LOG_INFO("Waiting for @tablet_state != transient of table %Qv", Path);
            Sleep(TDuration::Seconds(1));
        }
}

ui32 TTableBase::TabletCount() const {
    return static_cast<ui32>(GetYtAttr<ui64>(Client, Path, "tablet_count"));
}

TTableBase::TPivotKeys TTableBase::PivotKeys() const {
    auto tablets = GetTablets();
    TPivotKeys retval;
    retval.reserve(tablets.size());
    for (const auto& tablet : tablets) {
        TOwningKey key;
        Deserialize(key, tablet->AsMap()->GetChildOrThrow("pivot_key"));
        retval.emplace_back(std::move(key));
    }
    return retval;
}

i32 TTableBase::TabletIndex(TUnversionedRow row) const {
    const auto keyColumnCount = GetSchema().GetKeyColumnCount();

    auto pivotKeys = PivotKeys();

    auto it = std::upper_bound(
        pivotKeys.begin(),
        pivotKeys.end(),
        row,
        [&] (TUnversionedRow lhs, TUnversionedOwningRow rhs) {
            return CompareRows(lhs, rhs, keyColumnCount) < 0;
        });
    return static_cast<i32>(it - pivotKeys.begin()) -1;
}

void TTableBase::SetTransaction(NApi::ITransactionPtr tx) {
    if (tx) {
        Transaction = MakeWeak(std::move(tx));
        if (!GetTransaction()) {
            YT_LOG_WARNING(
                "Transaction object was destroyed immediately after SetTransaction(tx). "
                "Consider taking ownership over transaction.");
        }
    } else {
        Transaction.Reset();
    }
}

void TTableBase::SetClient(NApi::IClientPtr client) {
    Client = std::move(client);
    Transaction.Reset();
}

NApi::ITransactionPtr TTableBase::GetTransaction() const {
    return Transaction.Lock();
}

NApi::IClientBasePtr TTableBase::GetClientBase() const {
    if (auto transaction = GetTransaction()) {
        return transaction;
    } else {
        return Client;
    }
}

TTimestamp TTableBase::GetTimestamp(ETimestampAttribute attribute) const
{
    return GetAttribute<TTimestamp>(::ToString(attribute));
}

TString TTableBase::PutThisIntoQuery(TString const& query) const {
    if (TString::npos != query.find("$(this)")) {
        TPatternFormatter formatter;
        formatter.AddProperty("this", GetPath());
        return formatter.Format(query);
    } else {
        return query;//rely on shared storage
    }
}

} // NProtoApi
} // NYT
