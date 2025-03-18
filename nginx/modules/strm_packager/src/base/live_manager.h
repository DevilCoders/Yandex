#pragma once

#include <nginx/modules/strm_packager/src/base/logger.h>
#include <nginx/modules/strm_packager/src/base/shm_zone.h>

#include <nginx/modules/strm_packager/src/proto/live_data.pb.h>

#include <strm/trns_manager/proto/api/liveinfo/liveinfo.grpc.pb.h>

#include <infra/yp_service_discovery/api/api.pb.h>
#include <infra/yp_service_discovery/api/api.grpc.pb.h>

#include <util/datetime/base.h>
#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

namespace NStrm::NPackager {
    class TRequestWorker;

    class TLiveManager {
    private:
        struct TDiffBlockMeta {
            bool FullState : 1;
            ui64 Version : 63;
            ui32 Size;
            ngx_msec_t CreatedTime;
        };

    public:
        class TShmLiveData {
        private:
            friend class TLiveManager;
            friend class TShmZone<TShmLiveData>;
            using TZone = TShmZone<TShmLiveData>;

            // if FullState == 0 then next Size byte contains one NLiveDataProto::TDiff from Version-1 to Version
            // otherwise next Size byte contains several NLiveDataProto::TStreamState each preceding with its 32-bit size
            struct TShmDiffBlock {
                ngx_queue_t Node; // element of TState::DiffsList
                TDiffBlockMeta Meta;

                ui8 const* Data() const {
                    return ((ui8 const*)this) + sizeof(TShmDiffBlock);
                }
                ui8* Data() {
                    return ((ui8*)this) + sizeof(TShmDiffBlock);
                }

                static TShmDiffBlock* Node2Diff(ngx_queue_t* node) {
                    static_assert(offsetof(TShmDiffBlock, Node) == 0);
                    return (TShmDiffBlock*)node;
                }

                static TShmDiffBlock const* Node2Diff(ngx_queue_t const* node) {
                    static_assert(offsetof(TShmDiffBlock, Node) == 0);
                    return (TShmDiffBlock*)node;
                }
            };

            struct TState {
                bool Init;
                ngx_msec_t LastAccess;
                ngx_msec_t HealthTick;
                ui64 RestartCounter;

                ui64 MinVersion; // need for empty DiffsList, otherwise redundant
                ui64 Version;    // need for empty DiffsList, otherwise redundant

                // tail contain most resent version: ngx_queue_last(Diffs).Version == this->Version
                // head contain oldest version: ngx_queue_head(Diffs).Version == this->MinVersion
                ngx_queue_t DiffsList;

                TShmDiffBlock* FullState; // can be null
                bool FullStateRequired;

                bool Empty() const {
                    return ngx_queue_empty(&DiffsList);
                }

                struct TYPSDResolveCache {
                    i64 Version;
                    ui64 Size;
                    ui8* Data;
                };

                TYPSDResolveCache YPSDResolveCache;
            };

            void Init(TZone& zone);
            void InitWithExistingShmState(TZone& zone);

            TMaybe<TZone> Zone;
        };

        struct TSettings {
            ngx_msec_t ShmStoreTTL = 1000 * 10; // 10 seconds

            ngx_msec_t MaxGrpcWorkTime = 5;
            ngx_msec_t MaxHealthTickDelay = 100;
            ngx_msec_t OldStreamTimeout = 1000 * 60 * 60 * 24 * 3; // 3 days
            ngx_msec_t GrpcReconnectTimeout = 1000;

            ngx_msec_t LatencyLogPeriod = 10000;

            ngx_msec_t GrpcArgInitialReconnectBackoffMs = 1000;
            ngx_msec_t GrpcArgMaxReconnectBackoffMs = 10000;
            ngx_msec_t GrpcArgKeepaliveTimeoutMs = 10000;
            ngx_msec_t GrpcArgKeepaliveTimeMs = 5000;
            ngx_msec_t GrpcArgHttp2MinSentPingIntervalWithoutDataMs = 1000;

            struct TYouberTarget {
                // balancer will be used in case yp discovery not available
                // TODO: write cache discovery results in file and maybe remove balancer after that

                TStringBuf BalancerTarget; // e.g.: "youber-trns-manager-1.strm.yandex.net:7070"
                TStringBuf EndpointSet;    // e.g.: "strm-youber-trns-mgr-prod-1"

                TYouberTarget() = default;

                // can be constructed from
                // - "youber-trns-manager-1.strm.yandex.net:7070" - with empty EndpointSet
                // - "youber-trns-manager-1.strm.yandex.net:7070,strm-youber-trns-mgr-prod-1" - with both BalancerTarget and EndpointSet defined
                explicit TYouberTarget(const TStringBuf s) {
                    Y_ENSURE(!s.empty());
                    if (!s.TrySplit(',', BalancerTarget, EndpointSet)) {
                        BalancerTarget = s;
                        EndpointSet = {};
                    }
                }

                friend inline bool operator<(const TYouberTarget& a, const TYouberTarget& b) {
                    return std::pair(a.BalancerTarget, a.EndpointSet) < std::pair(b.BalancerTarget, b.EndpointSet);
                }
            };

            // target = host:port
            //
            TSet<TYouberTarget> YouberTargets;

            // yp service discovery common params:
            TStringBuf YPSDTarget;                            // sd.yandex.net:8081
            TSet<TStringBuf> YPSDClusters;                    // sas, man, vla, etc.
            ngx_msec_t GrpcYPSDRequestPeriod = 1000 * 60 * 5; // 5 minutes
            ngx_msec_t YPSDResolveTimeout = 500;

            ui64 YPSDResolveCacheBufferSize = 1024 * 1024 * 2; // 2Mb

            struct TYouberResolveFromConfig {
                void Update(const ngx_str_t& endpointSetId, const ngx_str_t& cluster, const ngx_str_t& ipv6, const ngx_str_t& port, const ngx_str_t& timestamp);

                // endpoint_set_id -> cluster_name -> endpoint_set
                TMap<TString, TMap<TString, ::NYP::NServiceDiscovery::NApi::TRspResolveEndpoints>> Endpoints;
            };

            TYouberResolveFromConfig YouberResolveFromConfig;
        };

        struct TTimerControl {
            bool ShiftTimer = false;
            bool ShortPeriod = false;
        };

        // will be called with info == nulptr in case requested uuid is not known
        using TSubscriberCallback = std::function<void(const NLiveDataProto::TStream& info)>;

    private:
        class TLogHolder {
        public:
            void InitLog(const TNgxLogger& logger, std::function<void(TNgxStreamLogger&)> logPrefix);
            TNgxStreamLogger LogCrit();
            TNgxStreamLogger LogError();
            TNgxStreamLogger LogWarn();

        private:
            TNgxStreamLogger Log(const ngx_uint_t level);
            TMaybe<TNgxLogger> Logger;
            std::function<void(TNgxStreamLogger&)> LogPrefix;
        };

        struct TUpdate {
            ui64 Version;
            NLiveDataProto::TDiff Diff;
        };

        class TSubscribeCleaner {
        public:
            TSubscribeCleaner(TLiveManager& liveManager, const TString& uuid, const ui64 chunkIndex);
            ~TSubscribeCleaner();

            void Disable();

        private:
            TLiveManager& LiveManager;
            const TString Uuid;
            const ui64 ChunkIndex;
            bool Disabled;
        };

        // <ui64 : chunkIndex, TSubscribeCleaner* : cleaner>
        using TSubKey = std::pair<ui64, TSubscribeCleaner*>;

        class TData {
        public:
            void UpdateAndWakeSubscribers(TVector<TUpdate>&& updates, TLogHolder& log);

        public:
            class TStreamData {
            public:
                TStreamData() = default;
                TStreamData(const TStreamData&) = delete;
                TStreamData& operator=(const TStreamData&) = delete;

                const NLiveDataProto::TStream& GetStream() const {
                    return Data.GetStream();
                }

            private:
                void WakeSubscribers(TVector<TSubscriberCallback>& tmp);

                NLiveDataProto::TStreamState Data;

                TMap<TSubKey, TSubscriberCallback> Subscribes;

                friend class TLiveManager;
            };

            ui64 Version = 0;

            // uuid to stream data
            TMap<TString, TStreamData> Streams;
        };

        // TODO: add thread for grpc readings
        class TWorkState {
        public:
            TWorkState(const ui64 restartCounterValue, const TSettings& settings);

            // must wait for ShutdownTick returning true before destructing
            ~TWorkState() = default;

            // read update by grpc
            TVector<TLiveManager::TUpdate> WorkTick(
                const ui64 startVersion,
                TTimerControl& timerControl,
                const TSettings& settings,
                TLogHolder& log);

            bool ShutdownTick();

            bool IsWorking() const {
                return Working;
            }

            ui64 GetTotalMessagesCount() const {
                return TotalMessagesCount;
            }

        public:
            const ui64 RestartCounterValue;
            const TInstant CreationTime;

        private:
            struct TGrpcTag {
                enum class EType: ui32 {
                    Unset,
                    LiveInfoStream,
                    YPSDCall,
                };

                EType Type = EType::Unset;
            };

            struct TYouberGrpcStream: TGrpcTag {
                using TLiveInfoService = ::liveinfo::LiveInfoService;
                using TSubscribeLiveInfoResponse = ::liveinfo::SubscribeLiveInfoResponse;

                const ngx_msec_t ReconnectTimeout;

                // for calls in `pgr://strm-youber-trns-mgr-testing-1` - so endpoint set will be resolved by our calls to ypsd
                // and we will be able to use round_robin
                const std::unique_ptr<TLiveInfoService::Stub> ResolverStub;

                // for calls in `youber-trns-manager.tst.strm.yandex.net:7070`
                // as a fallback in case ypsd not responding and we have no cached resolve data
                const std::unique_ptr<TLiveInfoService::Stub> BalancerStub;

                TMaybe<ngx_msec_t> DisconnectTime;

                TMaybe<::grpc::ClientContext> Context;
                std::unique_ptr<::grpc::ClientAsyncReader<TSubscribeLiveInfoResponse>> Reader;
                TMaybe<TSubscribeLiveInfoResponse> Response;

                const TString EndpointSet;
                const TString ResolverTarget;
                const TString BalancerTarget;
                TString CurrentTarget;
                ui64 Msgcount = 0;
            };

            struct TYPSDGrpc {
                using TService = ::NYP::NServiceDiscovery::NApi::TServiceDiscoveryService;
                using TRequest = ::NYP::NServiceDiscovery::NApi::TReqResolveEndpoints;
                using TResponse = ::NYP::NServiceDiscovery::NApi::TRspResolveEndpoints;

                const ngx_msec_t RetryTimeout;  // wait between unsuccessfull requests
                const ngx_msec_t UpdateTimeout; // wait between successfull requests

                // single stub for all requests
                // either to single balancer sd.yandex.net
                // or (TODO) to round robin of {sas,man,vla,msk,...}.sd.yandex.net resolved by fake resolver
                const std::unique_ptr<TService::Stub> Stub;

                struct TCall: TGrpcTag {
                    TMaybe<ngx_msec_t> NextTime; // on success it will be set to now+UpdateTimeout, on fail it will be set to now+RetryTimeout
                    TString RequestId;           // log on fail

                    TMaybe<::grpc::ClientContext> Context;
                    std::unique_ptr<::grpc::ClientAsyncResponseReader<TResponse>> Reader;
                    TMaybe<TResponse> Response;
                    TMaybe<::grpc::Status> Status;

                    ngx_msec_t EffectiveRetryTimeout;

                    const TString EndpointSet;
                    const TString Cluster;
                };

                const TVector<THolder<TCall>> Calls;
            };

            void ReCallYPSD();
            void ReConnectYouber(const i64 nowMs, const TSettings& settings, TLogHolder& log);

            void LogLatency(const bool atBatchEnd, TLogHolder& log);

            static TMaybe<NLiveDataProto::TDiff> Convert(const TYouberGrpcStream::TSubscribeLiveInfoResponse& data, TLogHolder& log);

            ::grpc::CompletionQueue GrpcQueue;

            THolder<TYPSDGrpc> YPSDGrpc;

            TVector<THolder<TYouberGrpcStream>> YouberGrpcStreams;

            ui64 TotalMessagesCount;

            TMaybe<i64> MaxLatency;
            ngx_msec_t LastLatencyLog;

            bool Working;
        };

    public:
        using TStreamData = TData::TStreamData;

        TLiveManager(TShmZone<TLiveManager::TShmLiveData>& shmZone, const TSettings& settings);

        ~TLiveManager() = default;

        void InitLog(const TNgxLogger& logger);

        void Tick(TTimerControl& timerControl);

        TStreamData* Find(const TString& uuid);

        void Subscribe(TRequestWorker& request, TStreamData& streamData, const ui64 chunkIndex, const TSubscriberCallback callback);

    private:
        using TShmDiffBlock = TShmLiveData::TShmDiffBlock;

        // single buffer of shm size for all TDiffBlock-s that can be used at once
        // reset before every use
        struct TDiffBlockBuffer {
            TBuffer Buffer;
            ui8* pos = nullptr;
            ui8* end = nullptr;

            void Reset(const size_t size) {
                Buffer.Reserve(size);
                pos = (ui8*)Buffer.Data();
                end = (ui8*)Buffer.Data() + Buffer.Capacity();
            }

            bool Empty() const {
                return pos == (ui8*)Buffer.Data();
            }

            ui8* Alloc(const size_t size) {
                Y_ENSURE(pos + size <= end);
                ui8* result = pos;
                pos += size;
                return result;
            }
        };

        // diff block for temp storage of diffs outside of shm zone
        struct TDiffBlock {
        public:
            TDiffBlockMeta Meta;
            ui8* Data;

        public:
            static TDiffBlock Make(const ui32 size, TDiffBlockBuffer& buffer) {
                TDiffBlock result;
                result.Meta.CreatedTime = ngx_current_msec;
                result.Meta.Size = size;
                result.Meta.FullState = false;
                result.Meta.Version = 0;
                result.Data = buffer.Alloc(size);
                return result;
            }

            static TDiffBlock CloneShmBlock(const TShmDiffBlock& shmBlock, TDiffBlockBuffer& buffer) {
                TDiffBlock result = Make(shmBlock.Meta.Size, buffer);
                result.Meta = shmBlock.Meta;
                std::memcpy((void*)result.Data, shmBlock.Data(), shmBlock.Meta.Size);
                return result;
            }
        };

    private:
        // read updates from shm data, mutex must be already locked
        TVector<TDiffBlock> ReadDiffBlocks();

        static TVector<TUpdate> ParseDiffBlocks(const TVector<TDiffBlock>& diffBlocks);

        // these updates must NOT yet be applied to this->Data
        TVector<TDiffBlock> MakeDiffBlocks(const TVector<TUpdate>& updates, const bool fullStateRequired) const;

        // write in shm data updates, mutex must be already locked
        void WriteUpdates(const TVector<TDiffBlock>& diffBlocks);

        // remove streams that was not updated too long
        void RemoveOldStreams();

        // trying to decynchronize processes so they will not lock shm simultaneously
        static void ManageShiftTimer(bool& shiftTimer, TShmLiveData::TState& state);

        // WorkState must be defined and mutex must be already locked
        bool IsLeadTakenAway() {
            const TShmLiveData::TState& state = ShmDataZone.GetShmState();
            return !state.Init || state.RestartCounter != WorkState->RestartCounterValue;
        }

        void WorkShutDownTick() {
            if (WorkState->IsWorking()) {
                Logger.LogError() << "lead was taken away, begin shutdown";
            }
            if (WorkState->ShutdownTick()) {
                Logger.LogError() << "shutdown completed";
                WorkState.Clear();
            }
        }

    private:
        TShmZone<TShmLiveData> ShmDataZone;
        const TSettings Settings;

        ngx_msec_t LastOldStreamCheckTime;

        TData Data;

        TMaybe<TWorkState> WorkState;

        TLogHolder Logger;
        const TInstant CreationTime;

        mutable TDiffBlockBuffer DiffBlockBuffer;

    private:
        bool GrpcInit;
        // unused grpc object, hold just to prevent grpc from cleanup after TWorkState shutdown
        std::unique_ptr<::liveinfo::LiveInfoService::Stub> _GrpcAnchor;
    };

}
