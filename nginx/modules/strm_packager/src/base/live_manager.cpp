#include <nginx/modules/strm_packager/src/base/live_manager.h>

#include <nginx/modules/strm_packager/src/base/workers.h>

#include <nginx/modules/strm_packager/src/base/grpc_resolver.h>

#include <util/system/hp_timer.h>

namespace NStrm::NPackager {
    static inline bool operator==(const NLiveDataProto::TTranscoderData& a, const NLiveDataProto::TTranscoderData& b) {
        return a.GetHttpPort() == b.GetHttpPort() && a.GetHost() == b.GetHost();
    }

    void TLiveManager::TShmLiveData::Init(TZone& zone) {
        Y_ENSURE(!Zone);
        Zone.ConstructInPlace(zone);

        TState* state = (TState*)Zone->Alloc(sizeof(TState));
        Zone->SetShmState(state);

        state->Init = false;
        state->LastAccess = 0;
        state->RestartCounter = 0;
        state->MinVersion = 0;
        state->Version = 0;
        ngx_queue_init(&state->DiffsList);
        state->FullState = nullptr;
        state->FullStateRequired = false;

        state->YPSDResolveCache.Data = nullptr;
        state->YPSDResolveCache.Size = 0;
        state->YPSDResolveCache.Version = -1;
    }

    void TLiveManager::TShmLiveData::InitWithExistingShmState(TZone& zone) {
        Y_ENSURE(!Zone);
        Zone.ConstructInPlace(zone);
    }

    TLiveManager::TLiveManager(TShmZone<TLiveManager::TShmLiveData>& shmZone, const TSettings& settings)
        : ShmDataZone(shmZone)
        , Settings(settings)
        , LastOldStreamCheckTime(ngx_current_msec - settings.OldStreamTimeout - 1)
        , CreationTime(TInstant::Now())
        , GrpcInit(false)
    {
        Y_ENSURE(!Settings.YouberTargets.empty());

        Y_ENSURE(Settings.YPSDClusters.empty() == Settings.YPSDTarget.empty());
        for (const auto& target : Settings.YouberTargets) {
            Y_ENSURE(Settings.YPSDTarget.empty() == target.EndpointSet.empty());
            Y_ENSURE(!target.BalancerTarget.empty());
        }

        THPTimer().Passed(); // just to early init internal singletone
    }

    void TLiveManager::TSettings::TYouberResolveFromConfig::Update(
        const ngx_str_t& nsEndpointSetId,
        const ngx_str_t& nsCluster,
        const ngx_str_t& nsIpv6,
        const ngx_str_t& nsPort,
        const ngx_str_t& nsTimestamp) {
        const TString endpointSetId((char*)nsEndpointSetId.data, nsEndpointSetId.len);
        const TString cluster((char*)nsCluster.data, nsCluster.len);
        const TString ipv6((char*)nsIpv6.data, nsIpv6.len);
        const int port = FromString<int, char>((char*)nsPort.data, nsPort.len);
        const ui64 timestamp = FromString<ui64, char>((char*)nsTimestamp.data, nsTimestamp.len);

        Y_ENSURE(endpointSetId && cluster && ipv6 && port >= 0 && port <= 65536);

        ::NYP::NServiceDiscovery::NApi::TRspResolveEndpoints& es = Endpoints[endpointSetId][cluster];

        es.set_timestamp(timestamp);
        es.set_resolve_status(::NYP::NServiceDiscovery::NApi::EResolveStatus::OK);
        es.mutable_endpoint_set()->set_endpoint_set_id(endpointSetId);

        ::NYP::NServiceDiscovery::NApi::TEndpoint& ep = *es.mutable_endpoint_set()->add_endpoints();
        ep.set_id(endpointSetId);
        ep.set_ip6_address(ipv6);
        ep.set_port(port);
        ep.set_ready(true);

        NPackagerGrpcResolver::CheckAddress(ep);
    }

    TLiveManager::TWorkState::TWorkState(const ui64 restartCounterValue, const TSettings& settings)
        : RestartCounterValue(restartCounterValue)
        , CreationTime(TInstant::Now())
        , TotalMessagesCount(0)
        , LastLatencyLog(ngx_current_msec)
        , Working(true)
    {
        // now create grpc clients
        grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, settings.GrpcArgInitialReconnectBackoffMs);
        args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, settings.GrpcArgMaxReconnectBackoffMs);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, settings.GrpcArgKeepaliveTimeoutMs);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, settings.GrpcArgKeepaliveTimeMs);
        args.SetInt(GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, settings.GrpcArgHttp2MinSentPingIntervalWithoutDataMs);
        args.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

        grpc::ChannelArguments routRobinArgs = args;
        routRobinArgs.SetLoadBalancingPolicyName("round_robin");

        grpc::ChannelArguments ypsdArgs;
        ypsdArgs.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, settings.GrpcArgInitialReconnectBackoffMs);
        ypsdArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, settings.GrpcArgMaxReconnectBackoffMs);
        ypsdArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, settings.GrpcArgKeepaliveTimeoutMs);
        ypsdArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, settings.GrpcArgKeepaliveTimeMs);

        // clients for YP Service Discovery
        {
            TVector<THolder<TYPSDGrpc::TCall>> calls;
            for (const auto& target : settings.YouberTargets) {
                if (target.EndpointSet.empty()) {
                    continue;
                }

                for (const auto& cluster : settings.YPSDClusters) {
                    calls.push_back(THolder<TYPSDGrpc::TCall>(new TYPSDGrpc::TCall{
                        .EffectiveRetryTimeout = settings.GrpcReconnectTimeout,
                        .EndpointSet = (TString)target.EndpointSet,
                        .Cluster = (TString)cluster,
                    }));
                }
            }

            Y_ENSURE(settings.YPSDTarget.empty() == calls.empty());

            YPSDGrpc = THolder<TYPSDGrpc>(new TYPSDGrpc{
                .RetryTimeout = settings.GrpcReconnectTimeout,
                .UpdateTimeout = settings.GrpcYPSDRequestPeriod,
                .Stub = calls.empty()
                            ? nullptr
                            : TYPSDGrpc::TService::NewStub(
                                  grpc::CreateCustomChannel(
                                      (TString)settings.YPSDTarget,
                                      grpc::InsecureChannelCredentials(),
                                      ypsdArgs),
                                  grpc::StubOptions()),
                .Calls = std::move(calls),
            });
        }

        // clients for Youber
        for (const auto& target : settings.YouberTargets) {
            const TString resolverTarget = NPackagerGrpcResolver::Scheme + TString("://") + target.EndpointSet;

            YouberGrpcStreams.push_back(THolder<TYouberGrpcStream>(new TYouberGrpcStream{
                .ReconnectTimeout = settings.GrpcReconnectTimeout,

                .ResolverStub =
                    target.EndpointSet.empty()
                        ? nullptr
                        : TYouberGrpcStream::TLiveInfoService::NewStub(
                              grpc::CreateCustomChannel(
                                  resolverTarget,
                                  grpc::InsecureChannelCredentials(),
                                  routRobinArgs),
                              grpc::StubOptions()),

                .BalancerStub = TYouberGrpcStream::TLiveInfoService::NewStub(
                    grpc::CreateCustomChannel(
                        (TString)target.BalancerTarget,
                        grpc::InsecureChannelCredentials(),
                        args),
                    grpc::StubOptions()),

                .EndpointSet = (TString)target.EndpointSet,
                .ResolverTarget = resolverTarget,
                .BalancerTarget = (TString)target.BalancerTarget,
                .Msgcount = 0,
            }));
        }
    }

    bool TLiveManager::TWorkState::ShutdownTick() {
        Working = false;
        GrpcQueue.Shutdown();

        for (const THolder<TYPSDGrpc::TCall>& call : YPSDGrpc->Calls) {
            if (call->Reader) {
                call->Context->TryCancel();
            }
        }

        while (true) {
            void* gotTag = nullptr;
            bool ok = false;
            const grpc::CompletionQueue::NextStatus nextStatus = GrpcQueue.AsyncNext(
                &gotTag,
                &ok,
                ::gpr_timespec{
                    .tv_sec = 0,
                    .tv_nsec = 0,
                    .clock_type = GPR_TIMESPAN,
                });

            switch (nextStatus) {
                case grpc::CompletionQueue::NextStatus::SHUTDOWN:
                    return true;
                case grpc::CompletionQueue::NextStatus::TIMEOUT:
                    return false;
                case grpc::CompletionQueue::NextStatus::GOT_EVENT:
                    continue;
            }
        }
    }

    TMaybe<NLiveDataProto::TDiff> TLiveManager::TWorkState::Convert(const TYouberGrpcStream::TSubscribeLiveInfoResponse& response, TLogHolder& log) {
        TMaybe<NLiveDataProto::TDiff> result;

        if (response.has_batch_end()) {
            result.ConstructInPlace();
            result->MutableBatchEnd();
            return result;
        } else if (response.has_stream()) {
            result.ConstructInPlace();
            NLiveDataProto::TStream& stream = *result->MutableStream();

            const ::liveinfo::StreamState& data = response.stream();

            stream.SetUuid(data.stream_uuid());
            stream.SetS3Bucket(data.s3_bucket());

            const ui32 chunkDuration = data.chunk_duration();
            const i64 segmentDuration = data.segment_duration();
            const i64 partDuration = data.part_duration();

            Y_ENSURE(chunkDuration > 0);
            Y_ENSURE(segmentDuration > 0);
            Y_ENSURE(partDuration > 0);

            stream.SetChunkDuration(chunkDuration);
            stream.SetSegmentDuration(segmentDuration);
            stream.SetHLSPartDuration(partDuration);
            stream.SetChunkFragmentDuration(500); // TODO: must be from data!

            stream.SetFirstChunkIndex(data.first_chunk() / chunkDuration);
            stream.SetLastChunkIndex(data.last_chunk() / chunkDuration);
            stream.SetLastChunkInS3Index(data.last_uploaded_chunk_to_s3() / chunkDuration);
            stream.SetEnded(data.end() > 0);

            for (size_t i = 0; i < (size_t)data.transcoding_periods_size(); ++i) {
                const ::liveinfo::MasterTranscodingPeriod& period = data.transcoding_periods(i);

                const ui64 firstIndex = period.first_chunk_end_ts() / chunkDuration;
                const ui64 lastIndex = period.last_chunk_end_ts() / chunkDuration;

                // no need to store chunk periods that are completely in s3
                // but use `<` intead of `<=` in order to store last chunk info anyway
                // (to know master transcoder for low-latency chunk)
                if (lastIndex < stream.GetLastChunkInS3Index()) {
                    continue;
                }

                NLiveDataProto::TChunkRange& range = *stream.AddChunkRanges();

                range.SetChunkIndexBegin(firstIndex);
                range.SetChunkIndexEnd(lastIndex + 1);
                range.MutableTranscoder()->SetHost(period.master_transcoder());
                range.MutableTranscoder()->SetHttpPort(19350); // TODO: port must be from data
            }

            result->SetTimestamp(data.last_updated());

            return result;
        } else if (response.has_new_chunk()) {
            result.ConstructInPlace();
            NLiveDataProto::TNewChunk& newChunk = *result->MutableNewChunk();

            const ::liveinfo::NewChunk& data = response.new_chunk();

            newChunk.SetUuid(data.stream_uuid());
            newChunk.SetChunkEndTimestamp(data.chunk_end_timestamp());
            newChunk.MutableTranscoder()->SetHost(data.master_transcoder());
            newChunk.MutableTranscoder()->SetHttpPort(19350); // TODO: port must be from data

            result->SetTimestamp(data.timestamp());

            return result;
        } else if (response.has_chunk_uploaded()) {
            result.ConstructInPlace();
            NLiveDataProto::TChunkInS3& chunkInS3 = *result->MutableChunkInS3();

            const ::liveinfo::ChunkUploaded& data = response.chunk_uploaded();

            chunkInS3.SetUuid(data.stream_uuid());
            chunkInS3.SetChunkEndTimestamp(data.last_chunk_uploaded());

            result->SetTimestamp(data.timestamp());

            return result;
        } else if (response.has_stream_ended()) {
            result.ConstructInPlace();
            NLiveDataProto::TEndStream& endStream = *result->MutableEndStream();

            const ::liveinfo::StreamEnded& data = response.stream_ended();

            endStream.SetUuid(data.stream_uuid());
            endStream.SetLastChunkEndTimestamp(data.end_timestamp());

            result->SetTimestamp(data.timestamp());

            return result;
        } else if (response.has_stream_deleted()) {
            result.ConstructInPlace();
            NLiveDataProto::TDeleteStream& deleteStream = *result->MutableDeleteStream();

            const ::liveinfo::StreamDeleted& data = response.stream_deleted();

            deleteStream.SetUuid(data.stream_uuid());

            // not us for now - there is a bug - it is in seconds instead of milliseconds
            // result->SetTimestamp(data.timestamp());

            return result;
        } else if (response.has_quality_bandwidths()) {
            // just ignore, no use here
            return {};
        } else {
            log.LogError() << " cant convert response '" << ToString(response) << "', ignored";
            return {};
        }
    }

    void TLiveManager::TWorkState::LogLatency(const bool atBatchEnd, TLogHolder& log) {
        log.LogError() << " latency" << (atBatchEnd ? " [at batch end]" : "") << ": " << MaxLatency;
        LastLatencyLog = ngx_current_msec;
        MaxLatency.Clear();
    }

    void TLiveManager::TWorkState::ReCallYPSD() {
        for (const THolder<TYPSDGrpc::TCall>& call : YPSDGrpc->Calls) {
            if (!call->Reader && (call->NextTime.Empty() || ngx_msec_int_t(ngx_current_msec - *call->NextTime) > 0)) {
                call->Context.ConstructInPlace();
                call->Response.Clear();

                call->RequestId = TStringBuilder() << "strm-packager|" << call->Cluster << "|" << call->EndpointSet << "|" << ngx_random() << "|" << ::TInstant::Now();

                TYPSDGrpc::TRequest req;
                req.set_cluster_name(call->Cluster);
                req.set_endpoint_set_id(call->EndpointSet);
                req.set_client_name("strm-packager");
                req.set_ruid(call->RequestId);

                call->Type = TGrpcTag::EType::YPSDCall;

                call->Reader = YPSDGrpc->Stub->AsyncResolveEndpoints(
                    &*call->Context,
                    req,
                    &GrpcQueue);

                call->Response.ConstructInPlace();
                call->Status.ConstructInPlace();

                call->Reader->Finish(
                    &*call->Response,
                    &*call->Status,
                    call.Get()); // = TCall* as tag
            }
        }
    }

    void TLiveManager::TWorkState::ReConnectYouber(const i64 nowMs, const TSettings& settings, TLogHolder& log) {
        bool needResolver = false;

        for (const THolder<TYouberGrpcStream>& gs : YouberGrpcStreams) {
            if (!gs->Reader && (gs->DisconnectTime.Empty() || gs->Msgcount > 0 || ngx_msec_int_t(ngx_current_msec - (gs->ReconnectTimeout + *gs->DisconnectTime)) > 0)) {
                TYouberGrpcStream::TLiveInfoService::Stub* stub = gs->BalancerStub.get();
                gs->CurrentTarget = gs->BalancerTarget;

                if (gs->EndpointSet) {
                    const std::pair<bool, TInstant> st = NPackagerGrpcResolver::CheckEndpointSedIdResolved(gs->EndpointSet);
                    if (st.first) {
                        stub = gs->ResolverStub.get();
                        gs->CurrentTarget = gs->ResolverTarget;
                        needResolver = true;
                    } else if (nowMs < i64(st.second.MilliSeconds()) + i64(settings.YPSDResolveTimeout)) {
                        // wait for resolve
                        continue;
                    }
                }

                log.LogError() << "calling youber in " << gs->CurrentTarget;

                gs->Type = TGrpcTag::EType::LiveInfoStream;
                gs->Context.ConstructInPlace();
                gs->Response.Clear();

                ::liveinfo::SubscribeLiveInfoRequest emptyRequestDataIn;

                gs->Reader = stub->AsyncSubscribeOnLiveInfo(
                    &*gs->Context,
                    emptyRequestDataIn,
                    &GrpcQueue,
                    gs.Get()); // = TYouberGrpcStream* as tag

                // WARNING: do not use `gs->Reader->Finish(..)`
                //  it prevent proper shutdown (grpc bug?) - tag, given in
                //  Finish is never come out of GrpcQueue during shutdonw
                //  and queue never become drained

                gs->Msgcount = 0;
            }
        }

        if (needResolver && NPackagerGrpcResolver::GetRunningResolversCount() == 0) {
            // this is not Y_ENSURE because:
            //   1. not sure that resolver must be created without delay
            //   2. it will throw on every tick, spam in logs, and do nothing good
            log.LogCrit() << "pgr resolver required but not created";
        }
    }

    TVector<TLiveManager::TUpdate> TLiveManager::TWorkState::WorkTick(
        const ui64 startVersion,
        TTimerControl& timerControl,
        const TSettings& settings,
        TLogHolder& log)
    {
        Y_ENSURE(Working);
        TVector<TUpdate> updates;
        ui64 version = startVersion;

        THPTimer grpcWorkTimer;

        const i64 nowMs = TInstant::Now().MilliSeconds();

        // (re)call yp service dicovery
        ReCallYPSD();

        // (re)connect youber streams
        ReConnectYouber(nowMs, settings, log);

        while (true) {
            TGrpcTag* gotTag = nullptr;
            bool ok = false;
            const grpc::CompletionQueue::NextStatus nextStatus = GrpcQueue.AsyncNext(
                (void**)&gotTag,
                &ok,
                ::gpr_timespec{
                    .tv_sec = 0,
                    .tv_nsec = 0,
                    .clock_type = GPR_TIMESPAN,
                });

            if (nextStatus == grpc::CompletionQueue::NextStatus::TIMEOUT) {
                Y_ENSURE(!gotTag);
                break;
            }

            Y_ENSURE(nextStatus == grpc::CompletionQueue::NextStatus::GOT_EVENT);

            Y_ENSURE(gotTag);

            if (gotTag->Type == TGrpcTag::EType::YPSDCall) {
                TYPSDGrpc::TCall& call = *(TYPSDGrpc::TCall*)gotTag;

                Y_ENSURE(call.Reader);
                Y_ENSURE(call.Response.Defined());
                Y_ENSURE(call.Status.Defined());

                call.Reader = nullptr;

                call.NextTime = ngx_current_msec + YPSDGrpc->UpdateTimeout;
                if (!ok || call.Status->error_code() != ::grpc::OK) {
                    call.NextTime = ngx_current_msec + call.EffectiveRetryTimeout;
                    call.EffectiveRetryTimeout = std::min(YPSDGrpc->UpdateTimeout, call.EffectiveRetryTimeout * 2);

                    log.LogError() << "ypsd call failed " << call.RequestId << " ok: " << ok << " status: " << (int)call.Status->error_code() << " | " << call.Status->error_message();
                    continue;
                }

                call.EffectiveRetryTimeout = YPSDGrpc->RetryTimeout;

                if (call.Response->Getresolve_status() == NYP::NServiceDiscovery::NApi::EResolveStatus::NOT_EXISTS) {
                    log.LogCrit() << "ypsd call " << call.RequestId << " with resolve_status == NOT_EXISTS :: " << *call.Response;
                    continue;
                } else if (call.Response->Getresolve_status() == NYP::NServiceDiscovery::NApi::EResolveStatus::EMPTY) {
                    log.LogError() << "ypsd call " << call.RequestId << " with resolve_status == EMPTY :: " << *call.Response;
                } else {
                    log.LogError() << "ypsd call " << call.RequestId << " successful";
                }

                try {
                    NPackagerGrpcResolver::UpdateResolve(
                        call.EndpointSet,
                        call.Cluster,
                        *call.Response);
                } catch (...) {
                    log.LogCrit() << "update resolve with '" << ToString(*call.Response) << "' ruid = " << call.RequestId << " failed with exception '" << std::current_exception() << "'";
                }

            } else if (gotTag->Type == TGrpcTag::EType::LiveInfoStream) {
                TYouberGrpcStream& gs = *(TYouberGrpcStream*)gotTag;

                if (!ok) {
                    gs.Reader = nullptr;
                    gs.DisconnectTime = ngx_current_msec;

                    log.LogError() << "disconnected from " << gs.CurrentTarget << " msgcount = " << gs.Msgcount;
                    continue;
                }

                if (gs.Response.Defined()) {
                    ++gs.Msgcount;
                    ++TotalMessagesCount;

                    try {
                        TMaybe<NLiveDataProto::TDiff> diff = Convert(*gs.Response, log);

                        if (diff.Defined() && diff->HasBatchEnd()) {
                            LogLatency(/*atBatchEnd = */ true, log);
                        } else if (diff.Defined()) {
                            if (diff->GetTimestamp() > 0 && !(diff->HasStream() && diff->GetStream().GetEnded())) {
                                const i64 latency = nowMs - (i64)diff->GetTimestamp();
                                if (latency > MaxLatency) {
                                    MaxLatency = latency;
                                }
                            }

                            updates.emplace_back();
                            TUpdate& update = updates.back();

                            update.Version = ++version;
                            update.Diff = std::move(*diff);
                        }
                    } catch (...) {
                        log.LogCrit() << "convert response '" << ToString(*gs.Response) << "' from '" << gs.CurrentTarget << "' failed with exception: '" << std::current_exception() << "'";
                    }
                }

                gs.Response.ConstructInPlace();
                gs.Reader->Read(&*gs.Response, &gs);
            } else {
                Y_ENSURE(false);
            }

            if (grpcWorkTimer.Passed() * 1000 > settings.MaxGrpcWorkTime) {
                timerControl.ShortPeriod = true;
                break;
            }
        }

        if (ngx_msec_int_t(ngx_current_msec - LastLatencyLog) > (ngx_msec_int_t)settings.LatencyLogPeriod) {
            LogLatency(/*atBatchEnd = */ false, log);
        }

        return updates;
    }

    void TLiveManager::Tick(TTimerControl& timerControl) try {
        if (!GrpcInit) {
            NPackagerGrpcResolver::Init();
            _GrpcAnchor = ::liveinfo::LiveInfoService::NewStub(grpc::CreateCustomChannel("not-exist-object", grpc::InsecureChannelCredentials(), grpc::ChannelArguments()), grpc::StubOptions());
            GrpcInit = true;
        }

        bool startWork = false;
        TMaybe<ui64> restartCounterValue;

        if (WorkState.Empty() || !WorkState->IsWorking()) {
            DiffBlockBuffer.Reset(ShmDataZone.Size());

            const TShmMutex shmMutex = ShmDataZone.GetMutex();
            TGuard<TShmMutex> shmGuard(shmMutex);

            TShmLiveData::TState& state = ShmDataZone.GetShmState();

            ManageShiftTimer(timerControl.ShiftTimer, state);

            // check health tick in case we can start our worker
            if (WorkState.Empty() && (!state.Init || ngx_msec_int_t(ngx_current_msec - state.HealthTick - Settings.MaxHealthTickDelay) > 0)) {
                // take the lead
                Logger.LogError() << "taking the lead";

                if (!state.Init) {
                    Data.Version = Max(Data.Version, state.Version);
                }

                state.HealthTick = ngx_current_msec;
                restartCounterValue = ++state.RestartCounter;
                state.Init = true;
                startWork = true;
            }

            const TVector<TDiffBlock> diffBlocks = ReadDiffBlocks();

            shmGuard.Release();

            TVector<TUpdate> updates = ParseDiffBlocks(diffBlocks);
            Data.UpdateAndWakeSubscribers(std::move(updates), Logger);
            RemoveOldStreams();
        }

        if (startWork) {
            WorkState.ConstructInPlace(*restartCounterValue, Settings);

            // read ypsd cache at work start
            {
                const TShmMutex shmMutex = ShmDataZone.GetMutex();
                TGuard<TShmMutex> shmGuard(shmMutex);
                TShmLiveData::TState& state = ShmDataZone.GetShmState();

                if (state.YPSDResolveCache.Data && state.YPSDResolveCache.Size > 0) {
                    TVector<ui8> copy(state.YPSDResolveCache.Size);
                    std::memcpy(copy.data(), state.YPSDResolveCache.Data, state.YPSDResolveCache.Size);
                    const i64 version = state.YPSDResolveCache.Version;

                    shmGuard.Release();

                    NPackagerGrpcResolver::LoadFromProto(version, copy.data(), copy.size());
                }
            }

            // and update with resolves from config (there will be no rollbacks, because of timestamps check for each resolve result)
            for (const auto& [endpointSetId, clusters] : Settings.YouberResolveFromConfig.Endpoints) {
                for (const auto& [cluster, eps] : clusters) {
                    NPackagerGrpcResolver::UpdateResolve(
                        endpointSetId,
                        cluster,
                        eps);
                }
            }
        }

        if (WorkState.Defined() && WorkState->IsWorking()) {
            DiffBlockBuffer.Reset(ShmDataZone.Size());

            TVector<TUpdate> updates = WorkState->WorkTick(Data.Version, timerControl, Settings, Logger);
            const i64 resolveVersion = NPackagerGrpcResolver::GetDataVersion();

            const TShmMutex shmMutex = ShmDataZone.GetMutex();
            TGuard<TShmMutex> shmGuardFirst(shmMutex);

            TShmLiveData::TState& state = ShmDataZone.GetShmState();

            ManageShiftTimer(timerControl.ShiftTimer, state);

            if (IsLeadTakenAway()) {
                shmGuardFirst.Release();
                WorkShutDownTick(); // someone took the lead - initiate shutdown
            } else {
                // still leading
                state.HealthTick = ngx_current_msec;
                const bool fullStateRequired = state.FullStateRequired;

                const bool writeResolveCache = state.YPSDResolveCache.Size == 0 || state.YPSDResolveCache.Version < resolveVersion;

                shmGuardFirst.Release();

                const TVector<TDiffBlock> diffBlocks = MakeDiffBlocks(updates, fullStateRequired);

                TMaybe<NPackagerGrpcResolver::TDataProto> resolveCacheToSave;
                ui64 resolveCacheToSaveByteSize = 0;
                if (writeResolveCache) {
                    resolveCacheToSave = NPackagerGrpcResolver::SaveToProto();
                    resolveCacheToSaveByteSize = resolveCacheToSave->ByteSizeLong();
                    if (resolveCacheToSaveByteSize > Settings.YPSDResolveCacheBufferSize) {
                        resolveCacheToSave.Clear();
                        Logger.LogCrit() << "TLiveManager::Tick resolve cache larger than buffer " << resolveCacheToSaveByteSize << " > " << Settings.YPSDResolveCacheBufferSize;
                    }
                }

                TGuard<TShmMutex> shmGuardSecond(shmMutex);

                if (IsLeadTakenAway()) {
                    shmGuardSecond.Release();
                    WorkShutDownTick(); // someone took the lead - initiate shutdown
                } else {
                    if (resolveCacheToSave.Defined()) {
                        if (!state.YPSDResolveCache.Data) {
                            state.YPSDResolveCache.Data = ShmDataZone.AllocNoexcept(Settings.YPSDResolveCacheBufferSize);
                        }

                        if (!state.YPSDResolveCache.Data) {
                            Logger.LogCrit() << "TLiveManager::Tick can't allock " << Settings.YPSDResolveCacheBufferSize << " bytes buffer in shm for YPSD resolve cache";
                        } else {
                            state.YPSDResolveCache.Size = 0;
                            Y_ENSURE(resolveCacheToSave->SerializeWithCachedSizesToArray(state.YPSDResolveCache.Data) == state.YPSDResolveCache.Data + resolveCacheToSaveByteSize);
                            state.YPSDResolveCache.Size = resolveCacheToSaveByteSize;
                            state.YPSDResolveCache.Version = resolveCacheToSave->GetVersion();
                        }
                    }

                    WriteUpdates(diffBlocks);

                    shmGuardSecond.Release();

                    Data.UpdateAndWakeSubscribers(std::move(updates), Logger);
                    RemoveOldStreams();
                }
            }

        } else if (WorkState.Defined() && !WorkState->IsWorking()) {
            WorkShutDownTick();
        }

    } catch (...) {
        Logger.LogCrit() << "TLiveManager::Tick c++ exception: " << CurrentExceptionMessage();

        // drop shm state
        const TShmMutex shmMutex = ShmDataZone.GetMutex();
        TGuard<TShmMutex> shmGuard(shmMutex);
        TShmLiveData::TState& state = ShmDataZone.GetShmState();
        state.Init = false;
        while (!state.Empty()) {
            TShmDiffBlock* const head = TShmDiffBlock::Node2Diff(ngx_queue_head(&state.DiffsList));
            ngx_queue_remove(&head->Node);
            ShmDataZone.Free((ui8*)head);
        }

        state.MinVersion = state.Version;
        state.FullState = nullptr;

        if (state.YPSDResolveCache.Data) {
            ShmDataZone.Free(state.YPSDResolveCache.Data);
            state.YPSDResolveCache.Data = nullptr;
        }
        state.YPSDResolveCache.Version = -1;
        state.YPSDResolveCache.Size = 0;
    }

    // read updates from shm data, mutex must be already locked
    TVector<TLiveManager::TDiffBlock> TLiveManager::ReadDiffBlocks() {
        Y_ENSURE(DiffBlockBuffer.Empty());
        Y_ENSURE(DiffBlockBuffer.Buffer.Capacity() >= ShmDataZone.Size());

        TShmLiveData::TState& state = ShmDataZone.GetShmState();

        Y_ENSURE(Data.Version <= state.Version);

        ngx_queue_t const* begin = nullptr;

        if (Data.Version + 1 < state.MinVersion) {
            if (state.FullState) {
                begin = &state.FullState->Node;
            } else {
                state.FullStateRequired = true;
            }
        } else {
            for (
                ngx_queue_t const* node = ngx_queue_last(&state.DiffsList);
                node != ngx_queue_sentinel(&state.DiffsList);
                node = ngx_queue_prev(node)) {
                TShmDiffBlock const* const diffBlock = TShmDiffBlock::Node2Diff(node);

                if (diffBlock->Meta.Version <= Data.Version) {
                    break;
                }

                begin = node;
            }

            if (begin) {
                Y_ENSURE(TShmDiffBlock::Node2Diff(begin)->Meta.Version == Data.Version + 1);
            }
        }

        if (!begin) {
            return {};
        }

        TVector<TDiffBlock> result;
        ui64 resultVersion = Data.Version;
        for (
            ngx_queue_t const* node = begin;
            node != ngx_queue_sentinel(&state.DiffsList);
            node = ngx_queue_next(node)) {
            TShmDiffBlock const* diffBlock = TShmDiffBlock::Node2Diff(node);

            Y_ENSURE(diffBlock->Meta.Version >= resultVersion);
            if (diffBlock->Meta.Version == resultVersion) {
                // fullstate diffBlock will go after normal diffBlock with the same version, no need to read them
                Y_ENSURE(diffBlock->Meta.FullState);
                continue;
            }

            resultVersion = diffBlock->Meta.Version;
            result.push_back(TDiffBlock::CloneShmBlock(*diffBlock, DiffBlockBuffer));
        }

        return result;
    }

    void TLiveManager::TData::TStreamData::WakeSubscribers(TVector<TSubscriberCallback>& tmp) {
        auto beginIt = Subscribes.begin();
        auto endIt = Subscribes.lower_bound(TSubKey(Data.GetStream().GetLastChunkIndex() + 1, nullptr));

        tmp.clear();

        for (auto it = beginIt; it != endIt; ++it) {
            it->first.second->Disable(); // disable cleaner
            tmp.push_back(it->second);   // store callback;
        }

        Subscribes.erase(beginIt, endIt);

        // run callbacks after iterating and erasing in subscribers map, since
        //   that map can be changed during callbacks
        for (const auto& callback : tmp) {
            callback(Data.GetStream());
        }
    }

    TVector<TLiveManager::TDiffBlock> TLiveManager::MakeDiffBlocks(const TVector<TUpdate>& updates, const bool fullStateRequired) const {
        Y_ENSURE(DiffBlockBuffer.Empty());

        TVector<TDiffBlock> result;
        result.reserve(updates.size() + (fullStateRequired ? 1 : 0));

        // make full state block
        if (fullStateRequired) {
            ui64 size = 0;

            for (const auto& [_, stream] : Data.Streams) {
                const ui64 streamByteSize = stream.Data.ByteSizeLong();
                Y_ENSURE(streamByteSize > 0 && streamByteSize == (ui64)stream.Data.GetCachedSize()); // cached_size in protobuf is int
                size += streamByteSize + 4;
            }
            Y_ENSURE(size == ui32(size));

            TDiffBlock block = TDiffBlock::Make(size, DiffBlockBuffer);
            block.Meta.Version = Data.Version;
            block.Meta.FullState = true;

            ui8* dst = block.Data;

            for (const auto& [_, stream] : Data.Streams) {
                const ui32 streamByteSize = stream.Data.GetCachedSize();

                std::memcpy(dst, &streamByteSize, 4);
                dst += 4;

                Y_ENSURE(dst + streamByteSize == stream.Data.SerializeWithCachedSizesToArray(dst));
                dst += streamByteSize;
            }

            Y_ENSURE(dst == block.Data + block.Meta.Size);

            result.push_back(std::move(block));
        }

        for (const TUpdate& update : updates) {
            const ui64 size = update.Diff.ByteSizeLong();
            Y_ENSURE(size > 0 && size == (ui64)update.Diff.GetCachedSize()); // cached_size in protobuf is int

            TDiffBlock block = TDiffBlock::Make(size, DiffBlockBuffer);
            block.Meta.Version = update.Version;
            block.Meta.FullState = false;
            Y_ENSURE(block.Data + block.Meta.Size == update.Diff.SerializeWithCachedSizesToArray(block.Data));

            result.push_back(std::move(block));
        }

        return result;
    }

    void TLiveManager::WriteUpdates(const TVector<TDiffBlock>& diffBlocks) {
        if (diffBlocks.empty()) {
            return;
        }

        TShmLiveData::TState& state = ShmDataZone.GetShmState();

        const auto removeOldestDiff = [&state, this]() {
            Y_ENSURE(!state.Empty());
            TShmDiffBlock* const head = TShmDiffBlock::Node2Diff(ngx_queue_head(&state.DiffsList));
            if (state.FullState == head) {
                state.FullState = nullptr;
            }
            ngx_queue_remove(&head->Node);
            ShmDataZone.Free((ui8*)head);
            Y_ENSURE(!state.Empty());
            state.MinVersion = TShmDiffBlock::Node2Diff(ngx_queue_head(&state.DiffsList))->Meta.Version;
        };

        // clear too old
        for (;;) {
            if (state.Empty()) {
                break;
            }

            TShmDiffBlock* const head = TShmDiffBlock::Node2Diff(ngx_queue_head(&state.DiffsList));
            TShmDiffBlock* const last = TShmDiffBlock::Node2Diff(ngx_queue_last(&state.DiffsList));

            if (last->Meta.CreatedTime > head->Meta.CreatedTime + Settings.ShmStoreTTL) {
                removeOldestDiff();

            } else {
                break;
            }
        }

        // write new
        for (const TDiffBlock& ublock : diffBlocks) {
            const ui64 expectedUblockVersion = state.Version + (ublock.Meta.FullState ? 0 : 1);

            if (state.Empty()) {
                Y_ENSURE(ublock.Meta.Version >= expectedUblockVersion);
            } else {
                Y_ENSURE(ublock.Meta.Version == expectedUblockVersion);
            }

            const ui64 allockSize = ublock.Meta.Size + sizeof(TShmDiffBlock);

            TShmDiffBlock* rblock = nullptr;

            while (true) {
                rblock = (TShmDiffBlock*)ShmDataZone.AllocNoexcept(allockSize);
                if (rblock) {
                    break;
                }

                removeOldestDiff();
            }

            // copy data
            rblock->Meta = ublock.Meta;
            std::memcpy(rblock->Data(), ublock.Data, ublock.Meta.Size);

            // add to DiffsList
            if (state.Empty()) {
                state.MinVersion = ublock.Meta.Version;
            }
            ngx_queue_insert_tail(&state.DiffsList, &rblock->Node);
            state.Version = ublock.Meta.Version;

            if (rblock->Meta.FullState) {
                state.FullState = rblock;
            }
        }

        Y_ENSURE(state.Version == diffBlocks.back().Meta.Version);
        Y_ENSURE(state.MinVersion <= diffBlocks.front().Meta.Version);

        if (state.FullState) {
            state.FullStateRequired = false;
        }
    }

    // static
    TVector<TLiveManager::TUpdate> TLiveManager::ParseDiffBlocks(const TVector<TDiffBlock>& diffBlocks) {
        TVector<TUpdate> result;
        result.reserve(diffBlocks.size());

        for (const TDiffBlock& diffBlock : diffBlocks) {
            ui8 const* data = diffBlock.Data;
            ui32 size = diffBlock.Meta.Size;

            result.emplace_back();
            TUpdate& upd = result.back();
            upd.Version = diffBlock.Meta.Version;

            if (diffBlock.Meta.FullState) {
                auto& fullState = *upd.Diff.MutableFullState();

                while (size > 0) {
                    Y_ENSURE(size > 4);
                    ui32 singleSize;
                    std::memcpy(&singleSize, data, 4);
                    Y_ENSURE(singleSize + 4 <= size);

                    data += 4;
                    size -= 4;

                    Y_ENSURE(fullState.AddStreams()->ParseFromArray(data, singleSize));
                    data += singleSize;
                    size -= singleSize;
                }
            } else {
                Y_ENSURE(upd.Diff.ParseFromArray(data, size));
            }
        }

        return result;
    }

    void TLiveManager::TData::UpdateAndWakeSubscribers(TVector<TUpdate>&& updates, TLogHolder& log) {
        TVector<TSubscriberCallback> tmp;

        for (TUpdate& update : updates) {
            NLiveDataProto::TDiff& diff = update.Diff;

            if (diff.HasFullState()) {
                // NOTE: do not remove from Streams elements that not listed in this diff
                //   since full state is just state of leader, and it still can be incompleted
                NLiveDataProto::TFullState& fullState = *diff.MutableFullState();
                for (size_t i = 0; i < fullState.StreamsSize(); ++i) {
                    NLiveDataProto::TStreamState& stream = *fullState.MutableStreams(i);
                    const auto& uuid = stream.GetStream().GetUuid();
                    TStreamData& uuidStream = Streams[uuid];
                    uuidStream.Data.Swap(&stream);
                    uuidStream.WakeSubscribers(tmp);
                }

                Version = update.Version;
                continue;
            }

            // stream to call WakeSubscribers
            TStreamData* stream2wake = nullptr;

            try {
                if (diff.HasStream()) {
                    NLiveDataProto::TStream& streamProto = *diff.MutableStream();
                    const auto& uuid = streamProto.GetUuid();

                    TData::TStreamData* stream = &Streams[uuid];
                    stream->Data.SetLastChangeTime(ngx_current_msec);
                    stream->Data.MutableStream()->Swap(&streamProto);

                    stream2wake = stream;
                } else if (diff.HasNewChunk()) {
                    const auto& newChunk = diff.GetNewChunk();
                    const auto& uuid = newChunk.GetUuid();

                    TData::TStreamData* stream = Streams.FindPtr(uuid);
                    Y_ENSURE(stream, "TLiveManager::TData::Update: new chunk with unknown uuid");
                    Y_ENSURE(stream->Data.GetStream().GetChunkDuration() > 0);

                    const ui64 chunkIndex = newChunk.GetChunkEndTimestamp() / stream->Data.GetStream().GetChunkDuration();

                    auto& chunkRanges = *stream->Data.MutableStream()->MutableChunkRanges();

                    if (chunkRanges.size() > 0 && chunkRanges.rbegin()->GetTranscoder() == newChunk.GetTranscoder()) {
                        chunkRanges.rbegin()->SetChunkIndexEnd(chunkIndex + 1);
                    } else {
                        auto& range = *chunkRanges.Add();
                        range.SetChunkIndexBegin(chunkIndex);
                        range.SetChunkIndexEnd(chunkIndex + 1);
                        *range.MutableTranscoder() = newChunk.GetTranscoder();
                    }

                    stream->Data.MutableStream()->SetLastChunkIndex(chunkIndex);

                    stream->Data.SetLastChangeTime(ngx_current_msec);

                    stream2wake = stream;
                } else if (diff.HasChunkInS3()) {
                    const auto& chunkInS3 = diff.GetChunkInS3();
                    const auto& uuid = chunkInS3.GetUuid();

                    // no need to set stream2wake, since chunks availability is not changed
                    TData::TStreamData* stream = Streams.FindPtr(uuid);
                    Y_ENSURE(stream, "TLiveManager::TData::Update: chukn in s3 with unknown uuid");
                    Y_ENSURE(stream->Data.GetStream().GetChunkDuration() > 0);

                    const ui64 chunkIndex = chunkInS3.GetChunkEndTimestamp() / stream->Data.GetStream().GetChunkDuration();

                    stream->Data.MutableStream()->SetLastChunkInS3Index(chunkIndex);

                    auto& chunkRanges = *stream->Data.MutableStream()->MutableChunkRanges();

                    while (chunkRanges.size() > 0 && chunkRanges.begin()->GetChunkIndexEnd() < chunkIndex) {
                        chunkRanges.erase(chunkRanges.begin());
                    }

                    stream->Data.SetLastChangeTime(ngx_current_msec);
                } else if (diff.HasEndStream()) {
                    const auto& endStream = diff.GetEndStream();
                    const auto& uuid = endStream.GetUuid();

                    // no need to set stream2wake, since chunks availability is not changed
                    TData::TStreamData* stream = Streams.FindPtr(uuid);
                    Y_ENSURE(stream, "TLiveManager::TData::Update: end stream with unknown uuid");

                    stream->Data.MutableStream()->SetEnded(true);

                    stream->Data.SetLastChangeTime(ngx_current_msec);
                } else if (diff.HasDeleteStream()) {
                    const auto& endStream = diff.GetEndStream();
                    const auto& uuid = endStream.GetUuid();

                    Streams.erase(uuid);
                } else {
                    Y_ENSURE(false, "TLiveManager::TData::Update unknown diff type");
                }
            } catch (...) {
                log.LogCrit() << "cant apply update '" << ToString(diff) << "' exception: '" << std::current_exception() << "'";
                stream2wake = nullptr;
            }

            Version = update.Version;

            // call WakeSubscribers must be outside of previous try-catch
            if (stream2wake) {
                stream2wake->WakeSubscribers(tmp);
            }
        }
    }

    TLiveManager::TSubscribeCleaner::TSubscribeCleaner(TLiveManager& liveManager, const TString& uuid, const ui64 chunkIndex)
        : LiveManager(liveManager)
        , Uuid(uuid)
        , ChunkIndex(chunkIndex)
        , Disabled(false)
    {
    }

    void TLiveManager::TSubscribeCleaner::Disable() {
        Disabled = true;
    }

    TLiveManager::TSubscribeCleaner::~TSubscribeCleaner() {
        if (!Disabled) {
            if (auto* sd = LiveManager.Data.Streams.FindPtr(Uuid)) {
                sd->Subscribes.erase(TSubKey(ChunkIndex, this));
            }
        }
    }

    TLiveManager::TData::TStreamData* TLiveManager::Find(const TString& uuid) {
        return Data.Streams.FindPtr(uuid);
    }

    void TLiveManager::Subscribe(TRequestWorker& request, TStreamData& streamData, const ui64 chunkIndex, const TSubscriberCallback callback) {
        if (streamData.Data.GetStream().GetLastChunkIndex() >= chunkIndex) {
            callback(streamData.Data.GetStream());
            return;
        }

        TSubscribeCleaner* cleaner = request.GetPoolUtil<TSubscribeCleaner>().New(*this, streamData.Data.GetStream().GetUuid(), chunkIndex);

        streamData.Subscribes[TSubKey(chunkIndex, cleaner)] = request.MakeIndependentCallback(callback);
    }

    void TLiveManager::ManageShiftTimer(bool& shiftTimer, TShmLiveData::TState& state) {
        if (state.LastAccess == ngx_current_msec) {
            shiftTimer = true;
        }
        state.LastAccess = ngx_current_msec;
    }

    void TLiveManager::RemoveOldStreams() {
        if (ngx_msec_int_t(ngx_current_msec - (LastOldStreamCheckTime + Settings.OldStreamTimeout)) <= 0) {
            return;
        }

        LastOldStreamCheckTime = ngx_current_msec;

        size_t removed = 0;
        for (auto it = Data.Streams.begin(); it != Data.Streams.end();) {
            if (ngx_msec_int_t(ngx_current_msec - (ngx_msec_t(it->second.Data.GetLastChangeTime()) + Settings.OldStreamTimeout)) > 0) {
                it = Data.Streams.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
    }

    void TLiveManager::TLogHolder::InitLog(const TNgxLogger& logger, std::function<void(TNgxStreamLogger&)> logPrefix) {
        Y_ENSURE(!Logger.Defined());
        Y_ENSURE(logPrefix);
        LogPrefix = logPrefix;
        Logger.ConstructInPlace(logger);
    }

    void TLiveManager::InitLog(const TNgxLogger& logger) {
        auto logPrefix = [this](TNgxStreamLogger& log) {
            log << "packager TLiveManager [CreationTime " << CreationTime << " Data.Version " << Data.Version << "]";
            if (WorkState.Defined()) {
                log << "[WorkState"
                    << " CreationTime " << WorkState->CreationTime
                    << " IsWorking " << (WorkState->IsWorking() ? 1 : 0)
                    << " RestartCounter " << WorkState->RestartCounterValue
                    << " TotalMessagesCount " << WorkState->GetTotalMessagesCount()
                    << "]";
            }
            log << ": ";
        };
        Logger.InitLog(logger, logPrefix);
    }

    TNgxStreamLogger TLiveManager::TLogHolder::LogCrit() {
        return Log(NGX_LOG_CRIT);
    }

    TNgxStreamLogger TLiveManager::TLogHolder::LogError() {
        return Log(NGX_LOG_ERR);
    }

    TNgxStreamLogger TLiveManager::TLogHolder::LogWarn() {
        return Log(NGX_LOG_WARN);
    }

    TNgxStreamLogger TLiveManager::TLogHolder::Log(const ngx_uint_t level) {
        Y_ENSURE(Logger.Defined());
        TNgxStreamLogger res = (*Logger)(level).Stream();
        LogPrefix(res);
        return res;
    }

}
