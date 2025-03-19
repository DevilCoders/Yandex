#pragma once

#include "public.h"

#include "rdma_target.h"
#include "storage_with_stats.h"

#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/rdma/public.h>
#include <cloud/blockstore/libs/service/public.h>
#include <cloud/blockstore/libs/service/storage.h>
#include <cloud/blockstore/libs/spdk/public.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <cloud/storage/core/libs/common/error.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TDiskAgentState
{
public:
    struct TSessionInfo
    {
        TString Id;
        TInstant LastActivityTs;
        ui64 MountSeqNumber = 0;
    };

private:
    struct TDeviceState
    {
        NProto::TDeviceConfig Config;
        std::shared_ptr<TStorageAdapter> StorageAdapter;
        TSessionInfo WriterSession;
        TVector<TSessionInfo> ReaderSessions;

        TStorageIoStatsPtr Stats;
    };

private:
    const TDiskAgentConfigPtr AgentConfig;
    const NSpdk::ISpdkEnvPtr Spdk;
    const ICachingAllocatorPtr Allocator;
    const IStorageProviderPtr StorageProvider;
    const IProfileLogPtr ProfileLog;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;

    ILoggingServicePtr Logging;
    NSpdk::ISpdkTargetPtr SpdkTarget;
    NRdma::IServerPtr RdmaServer;
    IRdmaTargetPtr RdmaTarget;

    THashMap<TString, TDeviceState> Devices;

    ui32 InitErrorsCount = 0;

public:
    TDiskAgentState(
        TDiskAgentConfigPtr agentConfig,
        NSpdk::ISpdkEnvPtr spdk,
        ICachingAllocatorPtr allocator,
        IStorageProviderPtr storageProvider,
        IProfileLogPtr profileLog,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        ILoggingServicePtr logging,
        NRdma::IServerPtr rdmaServer);

    struct TInitializeResult
    {
        TVector<NProto::TDeviceConfig> Configs;
        TVector<TString> Errors;
    };

    NThreading::TFuture<TInitializeResult> Initialize();

    NThreading::TFuture<NProto::TAgentStats> CollectStats();

    NThreading::TFuture<NProto::TReadDeviceBlocksResponse> Read(
        TInstant now,
        NProto::TReadDeviceBlocksRequest request);

    NThreading::TFuture<NProto::TWriteDeviceBlocksResponse> Write(
        TInstant now,
        NProto::TWriteDeviceBlocksRequest request);

    NThreading::TFuture<NProto::TZeroDeviceBlocksResponse> WriteZeroes(
        TInstant now,
        NProto::TZeroDeviceBlocksRequest request);

    NThreading::TFuture<NProto::TError> SecureErase(const TString& uuid, TInstant now);

    NThreading::TFuture<NProto::TChecksumDeviceBlocksResponse> Checksum(
        TInstant now,
        NProto::TChecksumDeviceBlocksRequest request);

    TString GetDeviceName(const TString& uuid) const;

    TVector<NProto::TDeviceConfig> GetDevices() const;

    void AcquireDevices(
        const TVector<TString>& uuids,
        const TString& sessionId,
        TInstant now,
        NProto::EVolumeAccessMode accessMode,
        ui64 mountSeqNumber);

    void ReleaseDevices(
        const TVector<TString>& uuids,
        const TString& sessionId);

    TSessionInfo GetWriterSession(const TString& uuid) const;
    TVector<TSessionInfo> GetReaderSessions(const TString& uuid) const;

    void StopTarget();

private:
    const TDeviceState& GetDeviceState(
        const TString& uuid,
        const TString& sessionId,
        const NProto::EVolumeAccessMode accessMode) const;

    const TDeviceState& GetDeviceStateImpl(
        const TString& uuid,
        const TString& sessionId,
        const NProto::EVolumeAccessMode accessMode) const;
    const TDeviceState& GetDeviceStateImpl(const TString& uuid) const;

    template <typename T>
    void WriteProfileLog(
        TInstant now,
        const TString& uuid,
        const T& req,
        ui32 blockSize,
        ESysRequestType requestType);
    
    NThreading::TFuture<TInitializeResult> InitSpdkStorage();
    NThreading::TFuture<TInitializeResult> InitAioStorage();

    void InitRdmaTarget();
};

}   // namespace NCloud::NBlockStore::NStorage
