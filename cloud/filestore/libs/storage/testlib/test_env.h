#pragma once

#include <cloud/filestore/libs/diagnostics/public.h>
#include <cloud/filestore/libs/storage/core/config.h>
#include <cloud/filestore/libs/storage/core/public.h>

#include <cloud/storage/core/libs/diagnostics/public.h>
#include <cloud/storage/core/libs/kikimr/public.h>

#include <ydb/core/base/tabletid.h>
#include <ydb/core/testlib/basics/appdata.h>
#include <ydb/core/testlib/basics/runtime.h>
#include <ydb/core/testlib/tablet_helpers.h>
#include <ydb/core/testlib/test_client.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/event.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

constexpr ui32 DefaultBlockCount = 268435456;   // TB
constexpr ui32 DefaultChannelCount = 7;

////////////////////////////////////////////////////////////////////////////////

struct TTestEnvConfig
{
    ui32 DomainUid = 1;
    TString DomainName = "local";

    ui32 StaticNodes = 1;
    ui32 DynamicNodes = 1;

    ui32 ChannelCount = DefaultChannelCount;
    ui32 Groups = 2;

    NActors::NLog::EPriority LogPriority_NFS = NActors::NLog::PRI_DEBUG;
    NActors::NLog::EPriority LogPriority_KiKiMR = NActors::NLog::PRI_WARN;
    NActors::NLog::EPriority LogPriority_Others = NActors::NLog::PRI_WARN;
};

////////////////////////////////////////////////////////////////////////////////

class TTestEnv
{
private:
    TTestEnvConfig Config;
    TStorageConfigPtr StorageConfig;

    NKikimr::TTestBasicRuntime Runtime;
    TVector<ui32> GroupIds;
    ui32 NextDynamicNode = 0;
    ui64 NextTabletId = 0;

    TMap<ui64, NKikimr::TTabletStorageInfo*> TabletIdToStorageInfo;

    IStorageCountersPtr StorageCounters;
    IRequestStatsRegistryPtr StatsRegistry;

public:
    TTestEnv(
        const TTestEnvConfig& config = {},
        NProto::TStorageConfig storageConfig = {});

    NActors::TTestActorRuntime& GetRuntime()
    {
        return Runtime;
    }

    TStorageConfigPtr GetStorageConfig() const
    {
        return StorageConfig;
    }

    ui64 GetHive();
    ui64 GetSchemeShard();

    const auto& GetGroupIds() const
    {
        return GroupIds;
    }

    ui64 AllocateTxId();
    void CreateSubDomain(const TString& name);
    ui32 CreateNode(const TString& name);

    ui64 BootIndexTablet(ui32 nodeIdx);
    void UpdateTabletStorageInfo(ui64 tabletId, ui32 channelCount);

private:
    void SetupLogging();

    void SetupDomain(NKikimr::TAppPrepare& app);
    void SetupChannelProfiles(NKikimr::TAppPrepare& app);

    std::unique_ptr<NKikimr::TTabletStorageInfo> BuildIndexTabletStorageInfo(
        ui64 tabletId,
        ui32 channelCount);

    void BootTablets();
    void BootStandardTablet(
        ui64 tabletId,
        NKikimr::TTabletTypes::EType type,
        ui32 nodeIdx = 0);

    void SetupStorage();

    void SetupLocalServices();
    void SetupLocalService(ui32 nodeIdx);

    void SetupLocalServiceConfig(
        NKikimr::TAppData& appData,
        NKikimr::TLocalConfig& localConfig);

    void SetupProxies();
    void SetupTicketParser(ui32 nodeIdx);
    void SetupTxProxy(ui32 nodeIdx);
    void SetupCompileService(ui32 nodeIdx);

    void InitSchemeShard();

    void WaitForSchemeShardTx(ui64 txId);
};

}   // namespace NCloud::NFileStore::NStorage
