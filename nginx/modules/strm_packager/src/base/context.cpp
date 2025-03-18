extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/base/context.h>
#include <nginx/modules/strm_packager/src/base/http_error.h>
#include <nginx/modules/strm_packager/src/base/log_levels.h>
#include <nginx/modules/strm_packager/src/base/shm_cache.h>
#include <nginx/modules/strm_packager/src/base/shm_zone.h>
#include <nginx/modules/strm_packager/src/base/workers.h>

#include <nginx/modules/strm_packager/src/temp/test_cache_worker.h>
#include <nginx/modules/strm_packager/src/temp/test_live.h>
#include <nginx/modules/strm_packager/src/temp/test_live_manager_subscribe_worker.h>
#include <nginx/modules/strm_packager/src/temp/test_mp4_vod.h>
#include <nginx/modules/strm_packager/src/temp/test_timed_meta.h>
#include <nginx/modules/strm_packager/src/temp/test_mpegts_vod.h>
#include <nginx/modules/strm_packager/src/temp/test_read_worker.h>
#include <nginx/modules/strm_packager/src/temp/test_worker.h>

#include <nginx/modules/strm_packager/src/workers/kaltura_imitation_worker.h>
#include <nginx/modules/strm_packager/src/workers/live_worker.h>
#include <nginx/modules/strm_packager/src/workers/music_worker.h>
#include <nginx/modules/strm_packager/src/workers/vod_worker.h>

#include <util/string/cast.h>
#include <util/system/hp_timer.h>

#define NGX_STRM_PACKAGER_CATCH_EX(request, todoblock)                     \
    catch (const TFatalExceptionContainer& fec) {                          \
        do {                                                               \
            todoblock                                                      \
        } while (0);                                                       \
        ExceptionLogAndFinalizeRequest(fec.Exception(), request);          \
        return NGX_OK;                                                     \
    }                                                                      \
    catch (...) {                                                          \
        do {                                                               \
            todoblock                                                      \
        } while (0);                                                       \
        ExceptionLogAndFinalizeRequest(std::current_exception(), request); \
        return NGX_OK;                                                     \
    }

#define NGX_STRM_PACKAGER_CATCH(request) NGX_STRM_PACKAGER_CATCH_EX(request, )

namespace NStrm::NPackager {
    extern "C" {
        extern ngx_module_t ngx_http_strm_packager_module_body_filter;
    }

    ngx_http_output_body_filter_pt TRequestContext::NextOutputBodyFilter = nullptr;
    ngx_http_output_header_filter_pt TRequestContext::NextOutputHeaderFilter = nullptr;

    template <typename TWorker>
    ngx_int_t TRequestContext::RequestHandler(ngx_http_request_t* request) try {
        Y_ENSURE(request == request->main);
        Y_ENSURE(!request->parent);
        request->main_filter_need_in_memory = 1;
        request->buffered |= BufferedFlag;
        Y_ENSURE(!RawGet(request));
        const TLocationConfig& config = *(TLocationConfig*)ngx_http_get_module_loc_conf(request, ngx_http_strm_packager_module_body_filter);
        TRequestContext* const context = TNgxPoolUtil<TRequestContext>::NewInPool(request->pool, request);
        SetContext(request, context);
        context->InRequestHandler = true;
        context->Worker = MakeHolder<TWorker>(*context, config);
        Y_ENSURE(config.Checked, "location " << config.ConfigFileLine << " not checked");
        context->Worker->Work();
        context->Worker->CreatePendingSubrequests();
        context->InRequestHandler = false;
        return NGX_OK;
    }
    NGX_STRM_PACKAGER_CATCH(request)

    void TRequestContext::StartFinalize() {
        FinalizeTimer.ResetTime(0);
    }

    TRequestContext::TRequestContext(ngx_http_request_t* request)
        : Request(request)
        , Aborting(false)
        , Finished(false)
        , InRequestHandler(false)
        , RunningSubrequests(0)
        , FinalizeTimer(request)
    {
        FlushChain = TNgxPoolUtil<ngx_chain_t>::Calloc(Request->pool);
        FlushChain->buf = TNgxPoolUtil<ngx_buf_t>::Calloc(Request->pool);
        FlushChain->buf->sync = 1;
        FlushChain->buf->flush = 1;

        FinishChain = TNgxPoolUtil<ngx_chain_t>::Calloc(Request->pool);
        FinishChain->buf = TNgxPoolUtil<ngx_buf_t>::Calloc(Request->pool);
        FinishChain->buf->sync = 1;
        FinishChain->buf->last_buf = 1;

        FinalizeTimer.ResetCallback([this]() {
            if (RunningSubrequests == 0 && Request->connection->data == Request && !Request->postponed) {
                ngx_connection_t* connection = Request->connection;
                ngx_http_set_log_request(connection->log, Request);

                const int status = FinalizeStatus.GetOrElse(NGX_OK);
                FinalizeStatus.Clear();

                Request->buffered &= ~BufferedFlag;

                ngx_http_finalize_request(Request, status);

                ngx_http_run_posted_requests(connection);
            } else {
                FinalizeTimer.ResetTime(500);
            }
        });
    }

    TRequestContext::~TRequestContext() {
        if (!Finished && !Aborting) {
            ngx_log_error(NGX_LOG_WARN, Request->connection->log, 0, "TRequestContext destroyed without finish!");
        }
    }

    inline TRequestContext* TRequestContext::RawGet(ngx_http_request_t* request) {
        return (TRequestContext*)ngx_http_get_module_ctx(request, ngx_http_strm_packager_module_body_filter);
    }

    inline TRequestContext* TRequestContext::Get(ngx_http_request_t* request) {
        TRequestContext* const context = RawGet(request);
        Y_ENSURE(context);
        return context;
    }

    void TRequestContext::SetContext(ngx_http_request_t* request, TRequestContext* context) {
        ngx_http_set_ctx(request, context, ngx_http_strm_packager_module_body_filter);
    }

    inline bool TRequestContext::IsSubrequest(ngx_http_request_t* request) {
        if (!request->parent) {
            return false;
        }
        TRequestContext* const context = RawGet(request->parent);
        if (!context) {
            return false;
        }
        Y_ENSURE(context->Request == request->parent);
        return true;
    }

    void TRequestContext::ExceptionLogAndFinalizeRequest(const std::exception_ptr exception, ngx_http_request_t* request) try {
        int status = NGX_HTTP_INTERNAL_SERVER_ERROR;
        ngx_uint_t logLevel = NGX_LOG_ERR;
        bool ngxSendError = false;

        try {
            std::rethrow_exception(exception);
        } catch (const std::exception& e) {
            yexception const* const ye = dynamic_cast<const yexception*>(&e);
            TString backtrace = ye ? (ye->BackTrace() ? ye->BackTrace()->PrintToString() : TString()) : TString();

            THttpError const* const he = dynamic_cast<const THttpError*>(&e);
            if (he) {
                status = he->Status;
                logLevel = ToNgxLogLevel(he->LogPriority);
                ngxSendError = he->NgxSendError;
            }
            ngx_log_error(logLevel, request->connection->log, 0, "fatal c++ exception [[ %s ]], backtrace [[ %s ]] ", e.what(), backtrace.c_str());
        } catch (...) {
            ngx_log_error(logLevel, request->connection->log, 0, "fatal c++ unknown exception");
        }

        TRequestContext* const context = TRequestContext::RawGet(request->main);

        if (context && context->Worker && !context->Aborting) {
            ngx_log_error(logLevel, request->connection->log, 0, "finalizing, status: %d header_sent: %d", status, context->Request->header_sent ? 1 : 0);
            context->Aborting = true;
            if (context->Request->header_sent && !ngxSendError) {
                status = NGX_ERROR;
            }

            context->FinalizeStatus = status;

            context->StartFinalize();
        } else {
            ngx_log_error(logLevel, request->connection->log, 0, "already finalized ? %d %d %d", context ? 1 : 0, (context && context->Worker) ? 1 : 0, (context && context->Aborting) ? 1 : 0);
        }
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, request->connection->log, 0, "fatal c++ exception at ExceptionLogAndFinalizeRequest [[ %s ]]", e.what());
        request->connection->error = 1;
    } catch (...) {
        ngx_log_error(NGX_LOG_ERR, request->connection->log, 0, "fatal c++ unknown exception at ExceptionLogAndFinalizeRequest");
        request->connection->error = 1;
    }

    ngx_int_t TRequestContext::SubrequestFinishedCallback(ngx_http_request_t* subrequest, void* data, ngx_int_t returnCode) try {
        Y_ENSURE(data);
        Y_ENSURE(IsSubrequest(subrequest));

        TRequestContext& context = *Get(subrequest->parent);
        Y_ENSURE(context.Worker);

        TPostSubrequestData& psrd = *(TPostSubrequestData*)data;

        if (psrd.Finished) {
            return NGX_OK;
        }

        psrd.Finished = true;
        --context.RunningSubrequests;

        if (context.Aborting) {
            return NGX_OK;
        }

        psrd.Worker.SubrequestFinished(ISubrequestWorker::TFinishStatus{.Code = returnCode, .GotLastInChain = psrd.GotLastInChain});
        context.Worker->CreatePendingSubrequests();
        return NGX_OK;
    }
    NGX_STRM_PACKAGER_CATCH(subrequest)

    template <typename TParam>
    ngx_int_t TRequestContext::GetVariableHandler(ngx_http_request_t* request, ngx_http_variable_value_t* var, uintptr_t offset) try {
        TRequestContext* const context = RawGet(request);
        if (!context) {
            var->not_found = 1;
            return NGX_OK;
        }

        const auto v = (TParam*)((char*)&context->Variables + offset);
        if (!v->Defined()) {
            var->not_found = 1;
            return NGX_OK;
        }
        const auto valueString = ToString(v->GetRef());
        ngx_str_t value = NginxString(valueString, request->pool);

        var->valid = 1;
        var->no_cacheable = 0;
        var->not_found = 0;
        var->len = value.len;
        var->data = value.data;
        return NGX_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, request->connection->log, 0, "c++ exception [[ %s ]] ", e.what());
        var->not_found = 1;
        return NGX_OK;
    } catch (...) {
        ngx_log_error(NGX_LOG_ERR, request->connection->log, 0, "c++ unknown exception");
        var->not_found = 1;
        return NGX_OK;
    }

    template <typename TParam, int n>
    constexpr ngx_http_variable_t MakeVariable(const char (&name)[n], TParam TVariables::*ParamField) {
        return {
            {n - 1, (u_char*)name},
            nullptr,
            TRequestContext::GetVariableHandler<TParam>,
            (size_t) & (((TVariables*)nullptr)->*ParamField),
            NGX_HTTP_VAR_CHANGEABLE,
            0,
        };
    }

    template <int n>
    constexpr ngx_http_variable_t MakeMetric(const char (&name)[n], TMaybe<float> TMetrics::*ParamField) {
        return {
            {n - 1, (u_char*)name},
            nullptr,
            TRequestContext::GetVariableHandler<TMaybe<float>>,
            (size_t) & (((TVariables*)nullptr)->Metrics.*ParamField),
            NGX_HTTP_VAR_CHANGEABLE,
            0,
        };
    }

    TVector<ngx_http_variable_t> TRequestContext::Vars = {
        MakeMetric("packager_is_cmaf", &TMetrics::IsCmaf),
        MakeMetric("packager_live_create_live_source_delay", &TMetrics::LiveCreateLiveSourceDelay),
        MakeMetric("packager_live_write_server_time_uuid_box", &TMetrics::LiveWriteServerTimeUuidBox),
        MakeMetric("packager_live_first_fragment_delay", &TMetrics::LiveFirstFragmentDelay),
        MakeMetric("packager_live_first_fragment_duration", &TMetrics::LiveFirstFragmentDuration),
        MakeMetric("packager_live_first_fragment_latency", &TMetrics::LiveFirstFragmentLatency),
        MakeMetric("packager_live_last_fragment_delay", &TMetrics::LiveLastFragmentDelay),
        MakeMetric("packager_live_last_fragment_duration", &TMetrics::LiveLastFragmentDuration),
        MakeMetric("packager_live_last_fragment_latency", &TMetrics::LiveLastFragmentLatency),
        MakeMetric("packager_live_is_low_latency", &TMetrics::LiveIsLowLatencyMode),
    };

    ngx_int_t TRequestContext::PreConfigurationBodyFilter(ngx_conf_t* cf) {
        for (auto& v : Vars) {
            auto var = ngx_http_add_variable(cf, &v.name, v.flags);
            if (var == NULL) {
                return NGX_ERROR;
            }

            var->get_handler = v.get_handler;
            var->data = v.data;
        }

        return NGX_OK;
    }

    ngx_int_t TRequestContext::PostConfigurationBodyFilter(ngx_conf_t* cf) {
        (void)cf;

        TMainConfig* mainConf = (TMainConfig*)ngx_http_conf_get_module_main_conf(cf, ngx_http_strm_packager_module_body_filter);

        if (mainConf->ShmLiveData.Defined()) {
            try {
                mainConf->LiveManager.ConstructInPlace(*mainConf->ShmLiveData, mainConf->LiveManagerSettings);
            } catch (...) {
                TNgxStreamLogger(NGX_LOG_EMERG, cf->log) << "LiveManager constructor failed with " << std::current_exception();
                return NGX_ERROR;
            }
        }

        for (TLocationConfig* const loc : mainConf->Locations) {
            try {
                if (loc->PackagerHandler.Empty()) {
                    continue;
                }

                Y_ENSURE(loc->WorkerChecker.Defined());
                Y_ENSURE(!loc->Checked);

                if (loc->LiveManagerAccess.GetOrElse(false)) {
                    Y_ENSURE(mainConf->LiveManager.Defined());
                    loc->LiveManager = &*mainConf->LiveManager;
                }

                loc->Check();
                loc->WorkerChecker.GetRef()(*loc);
                loc->Checked = true;
            } catch (...) {
                TNgxStreamLogger(NGX_LOG_EMERG, cf->log) << "check location " << loc->ConfigFileLine << " failed with " << std::current_exception();
                return NGX_ERROR;
            }
        }

        NextOutputBodyFilter = ngx_http_top_body_filter;
        ngx_http_top_body_filter = OutputBodyFilter;

        return NGX_OK;
    }

    ngx_int_t TRequestContext::PostConfigurationHeaderFilter(ngx_conf_t* cf) {
        (void)cf;

        NextOutputHeaderFilter = ngx_http_top_header_filter;
        ngx_http_top_header_filter = OutputHeaderFilter;

        return NGX_OK;
    }

    void* TRequestContext::CreateMainConfig(ngx_conf_t* cf) {
        return (void*)TNgxPoolUtil<TMainConfig>::NewInPool(cf->pool);
    }

    ngx_int_t TRequestContext::OnInitProcess(ngx_cycle_t* cycle) {
        ngx_http_conf_ctx_t* const httpCtx = (ngx_http_conf_ctx_t*)ngx_get_conf(cycle->conf_ctx, ngx_http_module);

        if (!httpCtx) {
            return NGX_OK;
        }

        TMainConfig* const mainConf = (TMainConfig*)httpCtx->main_conf[ngx_http_strm_packager_module_body_filter.ctx_index];

        if (!mainConf->LiveManager) {
            return NGX_OK;
        }

        mainConf->LiveManager->InitLog(TNgxLogger(NGX_LOG_CRIT, cycle->log));

        mainConf->EternalTimerEvent.log = cycle->log;
        mainConf->EternalTimerEvent.data = mainConf;
        mainConf->EternalTimerEvent.handler = &TRequestContext::EternalTimerEventHandler;
        mainConf->EternalTimerEvent.cancelable = 1;

        EternalTimerEventHandler(&mainConf->EternalTimerEvent);

        return NGX_OK;
    }

    void TRequestContext::EternalTimerEventHandler(ngx_event_t* event) {
        TLiveManager::TTimerControl timerControl;
        TMainConfig* const mainConf = (TMainConfig*)event->data;

        THPTimer hptimer;

        try {
            if (mainConf->LiveManager.Defined()) {
                mainConf->LiveManager->Tick(timerControl);
            }
        } catch (...) {
            TString msg = CurrentExceptionMessage();
            ngx_log_error(NGX_LOG_CRIT, event->log, 0, "packager eternal timer c++ exception [[ %s ]] ", msg.c_str());
        }

        // reset timer
        ngx_msec_t timeOffset = mainConf->EternalTimerPeriod;
        if (timerControl.ShortPeriod) {
            timeOffset = mainConf->EternalTimerShortPeriod;
        } else if (timerControl.ShiftTimer) {
            timeOffset += ngx_random() % mainConf->EternalTimerPeriod;
        }

        // ngx_add_timer use ngx_current_msec as current time, that is incorrect in case we spent much time working here
        //  so adjusting with hptimer
        timeOffset += hptimer.Passed() * 1000;
        ngx_add_timer(event, timeOffset);
    }

    void* TRequestContext::CreateLocationConfig(ngx_conf_t* cf) try {
        return TNgxPoolUtil<TLocationConfig>::NewInPool(
            cf->pool,
            TLocationConfig{
                .ConfigFileLine = TStringBuilder() << TStringBuf((char*)cf->conf_file->file.name.data, cf->conf_file->file.name.len) << ":" << cf->conf_file->line,
            });
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "c++ exception [[ %s ]] ", e.what());
        return NULL;
    }

    char* TRequestContext::MergeLocationConfig(ngx_conf_t* cf, void* parent, void* child) try {
        (void)cf;

        ((TLocationConfig*)child)->MergeFrom(*(TLocationConfig*)parent);

        TRequestContext::TMainConfig* mainConf = (TRequestContext::TMainConfig*)ngx_http_conf_get_module_main_conf(cf, ngx_http_strm_packager_module_body_filter);
        Y_ENSURE(mainConf);

        mainConf->Locations.emplace((TLocationConfig*)parent);
        mainConf->Locations.emplace((TLocationConfig*)child);

        return NGX_CONF_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_EMERG, cf->log, 0, "c++ exception [[ %s ]] ", e.what());
        return (char*)NGX_CONF_ERROR;
    }

    static inline void MarkChainAsRead(ngx_chain_t* chain) {
        for (ngx_chain_t* ce = chain; ce; ce = ce->next) {
            if (ce->buf) {
                ce->buf->pos = ce->buf->last;
                ce->buf->file_pos = ce->buf->file_last;
            }
        }
    }

    ngx_int_t TRequestContext::OutputBodyFilter(ngx_http_request_t* request, ngx_chain_t* chain) try {
        if (IsSubrequest(request)) {
            const TRequestContext& context = *Get(request->parent);
            if (context.Aborting) {
                MarkChainAsRead(chain);
                return NGX_OK;
            }
            Y_ENSURE(context.Worker);
            // context.Finished can be already set. it is ok that part of subrequest data is not needed for main request finish

            Y_ENSURE(request->post_subrequest);
            Y_ENSURE(request->post_subrequest->handler == TRequestContext::SubrequestFinishedCallback);
            Y_ENSURE(request->post_subrequest->data);

            TPostSubrequestData& psrd = *(TPostSubrequestData*)request->post_subrequest->data;
            Y_ENSURE(psrd.Headers && !psrd.Finished);

            for (ngx_chain_t* ce = chain; ce; ce = ce->next) {
                Y_ENSURE(ce->buf);

                Y_ENSURE(!ce->buf->last_buf); // last_buf must not be set in subrequest

                psrd.DataSize += ce->buf->last - ce->buf->pos;

                if (ce->buf->last_in_chain || psrd.GotLastInChain) {
                    // last_in_chain can be more than one, but there must be no data after first last_in_chain
                    Y_ENSURE(!psrd.GotLastInChain || psrd.DataSize == psrd.DataSizeOnLastInChain);
                    psrd.DataSizeOnLastInChain = psrd.DataSize;
                    psrd.GotLastInChain = true;
                }

                if (!ce->buf->pos || !ce->buf->last || ce->buf->last == ce->buf->pos) {
                    Y_ENSURE(ce->buf->last == ce->buf->pos);
                    Y_ENSURE(ce->buf->file_pos == ce->buf->file_last);
                    continue;
                }
                Y_ENSURE(ce->buf->last > ce->buf->pos);

                // send data to subrequest worker
                psrd.Worker.AcceptData((char*)ce->buf->pos, (char*)ce->buf->last);

                // mark buffer as read (need for copy_filter_module, that limit number of buffers in use - option 'output_buffers' in config)
                ce->buf->pos = ce->buf->last;
                ce->buf->file_pos = ce->buf->file_last;
            }

            context.Worker->CreatePendingSubrequests();

            return NGX_OK;
        }
        return NextOutputBodyFilter(request, chain);
    }
    NGX_STRM_PACKAGER_CATCH_EX(request, MarkChainAsRead(chain);)

    ngx_int_t TRequestContext::OutputHeaderFilter(ngx_http_request_t* request) try {
        if (IsSubrequest(request)) {
            const TRequestContext& context = *Get(request->parent);
            if (context.Aborting) {
                return NGX_OK;
            }

            Y_ENSURE(context.Worker);

            if (context.Finished) {
                // context.Finished can be already set. some subrequest can be not needed at all for main request finish
                //   e.g. source requested interval larger than actually will be needed for result,
                //   and that interval can be fetched in several subrequests, so some of them can be completely ignored
                context.Worker->LogWarn() << "subrequest completely ignored";
            }

            Y_ENSURE(request->post_subrequest);
            Y_ENSURE(request->post_subrequest->handler == TRequestContext::SubrequestFinishedCallback);
            Y_ENSURE(request->post_subrequest->data);
            TPostSubrequestData& psrd = *(TPostSubrequestData*)request->post_subrequest->data;
            Y_ENSURE(!psrd.Headers && !psrd.Finished);
            psrd.Headers = true;

            psrd.Worker.AcceptHeaders(THeadersOut(request->headers_out));
            context.Worker->CreatePendingSubrequests();

            return NGX_OK;
        }
        return NextOutputHeaderFilter(request);
    }
    NGX_STRM_PACKAGER_CATCH(request)

    template <typename TWorker>
    char* HandlerSetter(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) try {
        (void)cmd;
        (void)conf;
        TLocationConfig* const locationConf = (TLocationConfig*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_strm_packager_module_body_filter);
        Y_ENSURE(locationConf);

        TRequestContext::TMainConfig* mainConf = (TRequestContext::TMainConfig*)ngx_http_conf_get_module_main_conf(cf, ngx_http_strm_packager_module_body_filter);
        Y_ENSURE(mainConf);

        ngx_http_core_loc_conf_t* coreLocationConfiguration = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
        Y_ENSURE(coreLocationConfiguration);
        Y_ENSURE(!coreLocationConfiguration->handler, "more that one handler enabled at location '" + locationConf->ConfigFileLine + "'");
        coreLocationConfiguration->handler = TRequestContext::RequestHandler<TWorker>;

        locationConf->PackagerHandler = coreLocationConfiguration->handler;
        locationConf->WorkerChecker = &TWorker::CheckConfig;

        mainConf->Locations.emplace(locationConf);

        return NGX_CONF_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "c++ exception [[ %s ]] ", e.what());
        return (char*)NGX_CONF_ERROR;
    }

    template <typename TWorker, int n>
    constexpr ngx_command_t SetHandler(const char (&name)[n]) {
        return {
            {n - 1, (u_char*)name},
            NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
            HandlerSetter<TWorker>,
            0,
            0,
            nullptr};
    }

    template <typename T>
    void SetValue(TMaybe<TShmZone<T>>& dst, ngx_str_t& src, ngx_conf_t* cf) {
        Y_ENSURE(src.data);
        Y_ENSURE(src.len > 0);
        dst.ConstructInPlace(TShmZone<T>::GetExisting(src, cf));
    }

    void SetValue(TString& dst, const ngx_str_t& src, ngx_conf_t* /* cf*/) {
        Y_ENSURE(src.data);
        Y_ENSURE(src.len > 0);
        dst = TString((const char*)src.data, src.len);
    }

    void SetValue(TComplexValue& dst, const ngx_str_t& /* src*/, ngx_conf_t* cf) {
        ngx_http_complex_value_t* value = nullptr;
        ngx_command_t cvCommand;
        cvCommand.offset = 0;
        auto res = ngx_http_set_complex_value_slot(cf, &cvCommand, &value);
        Y_ENSURE(res == NGX_CONF_OK);
        dst = *value;
    }

    template <typename T>
    void SetValue(T& dst, const ngx_str_t& src, ngx_conf_t* /* cf*/) {
        Y_ENSURE(src.data);
        Y_ENSURE(src.len > 0);
        dst = FromString<T, char>((char*)src.data, src.len);
    }

    void SetValue(Ti64TimeMs& dst, const ngx_str_t& src, ngx_conf_t* /* cf*/) {
        Y_ENSURE(src.data);
        Y_ENSURE(src.len > 0);
        dst = Ti64TimeMs(FromString<i64, char>((char*)src.data, src.len));
    }

    void SetValue(ui64& dst, const ngx_str_t& src, ngx_conf_t* /* cf*/) {
        Y_ENSURE(src.data);
        Y_ENSURE(src.len > 0);
        ui64 multipllier = 1;
        switch (src.data[src.len - 1]) {
            case 'K':
                multipllier = 1024;
                break;
            case 'M':
                multipllier = 1024 * 1024;
                break;
            case 'G':
                multipllier = 1024 * 1024 * 1024;
                break;
        }
        dst = FromString<ui64, char>((char*)src.data, src.len - ((multipllier > 1) ? 1 : 0)) * multipllier;
    }

    template <typename T>
    void SetValue(TSet<T>& dst, const ngx_str_t& src, ngx_conf_t* /* cf*/) {
        Y_ENSURE(src.data);
        Y_ENSURE(src.len > 0);
        dst.emplace(TStringBuf((char const*)src.data, src.len));
    }

    template <typename T>
    void SetValue(TMaybe<T>& dst, const ngx_str_t& src, ngx_conf_t* cf) {
        if (!dst.Defined()) {
            dst.ConstructInPlace();
        }
        SetValue(*dst, src, cf);
    }

    template <typename TParam>
    char* ParamSetter(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) try {
        Y_ENSURE(conf);

        Y_ENSURE(cf->args->nelts == 2);

        SetValue(
            *(TParam*)((char*)conf + cmd->offset),
            ((ngx_str_t*)cf->args->elts)[1],
            cf);

        return NGX_CONF_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "c++ exception [[ %s ]] ", e.what());
        return (char*)NGX_CONF_ERROR;
    }

    template <>
    char* ParamSetter<TLiveManager::TSettings::TYouberResolveFromConfig>(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) try {
        Y_ENSURE(conf);

        Y_ENSURE(cf->args->nelts == 6);

        auto& field = *((TLiveManager::TSettings::TYouberResolveFromConfig*)((char*)conf + cmd->offset));
        field.Update(
            ((ngx_str_t*)cf->args->elts)[1],
            ((ngx_str_t*)cf->args->elts)[2],
            ((ngx_str_t*)cf->args->elts)[3],
            ((ngx_str_t*)cf->args->elts)[4],
            ((ngx_str_t*)cf->args->elts)[5]);

        return NGX_CONF_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "c++ exception [[ %s ]] ", e.what());
        return (char*)NGX_CONF_ERROR;
    }

    template <typename TParam, int n>
    constexpr ngx_command_t SetParam(const char (&name)[n], TParam TLocationConfig::*ParamField) {
        return {
            {n - 1, (u_char*)name},
            NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
            ParamSetter<TParam>,
            NGX_HTTP_LOC_CONF_OFFSET,
            (size_t) & (((TLocationConfig*)nullptr)->*ParamField), // offset
            nullptr};
    }

    template <typename TParam, int n>
    constexpr ngx_command_t SetParam(const char (&name)[n], TParam TRequestContext::TMainConfig::*ParamField) {
        return {
            {n - 1, (u_char*)name},
            NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
            ParamSetter<TParam>,
            NGX_HTTP_MAIN_CONF_OFFSET,
            (size_t) & (((TRequestContext::TMainConfig*)nullptr)->*ParamField), // offset
            nullptr};
    }

    template <typename TParam, int n>
    constexpr ngx_command_t SetParam(const char (&name)[n], TParam TLiveManager::TSettings::*ParamField) {
        return {
            {n - 1, (u_char*)name},
            NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
            ParamSetter<TParam>,
            NGX_HTTP_MAIN_CONF_OFFSET,
            (size_t) & (((TRequestContext::TMainConfig*)nullptr)->LiveManagerSettings.*ParamField), // offset
            nullptr};
    }

    template <int n>
    constexpr ngx_command_t SetParam(const char (&name)[n], TLiveManager::TSettings::TYouberResolveFromConfig TLiveManager::TSettings::*ParamField) {
        return {
            {n - 1, (u_char*)name},
            NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE5,
            ParamSetter<TLiveManager::TSettings::TYouberResolveFromConfig>,
            NGX_HTTP_MAIN_CONF_OFFSET,
            (size_t) & (((TRequestContext::TMainConfig*)nullptr)->LiveManagerSettings.*ParamField), // offset
            nullptr};
    }

    char* DeletedParamSetter(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {
        (void)cmd;
        (void)conf;
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "%V:%d command '%V' has no effect", &cf->conf_file->file.name, (int)cf->conf_file->line, ((ngx_str_t*)cf->args->elts) + 0);
        return NGX_CONF_OK;
    }

    template <int n>
    constexpr ngx_command_t DeletedParam(const char (&name)[n]) {
        return {
            {n - 1, (u_char*)name},
            NGX_ANY_CONF | NGX_CONF_ANY,
            DeletedParamSetter,
            0,
            0,
            nullptr};
    }

    char* ShmLiveDataZoneParamSetter(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) try {
        (void)cmd;
        Y_ENSURE(cf);
        Y_ENSURE(cf->args->nelts == 2);

        ngx_str_t& sizeStr = ((ngx_str_t*)cf->args->elts)[1];

        ssize_t size = ngx_parse_size(&sizeStr);
        Y_ENSURE(size != NGX_ERROR);

        TRequestContext::TMainConfig& mainConf = *(TRequestContext::TMainConfig*)conf;

        Y_ENSURE(!mainConf.ShmLiveData.Defined());

        TShmZone<TLiveManager::TShmLiveData> zone = TShmZone<TLiveManager::TShmLiveData>::CreateNew(ngx_string("pacakger_live_data_shm"), size, cf);

        mainConf.ShmLiveData.ConstructInPlace(zone);

        return NGX_CONF_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "c++ exception [[ %s ]] ", e.what());
        return (char*)NGX_CONF_ERROR;
    }

    template <int n>
    constexpr ngx_command_t SetShmLiveDataZoneParam(const char (&name)[n]) {
        return {
            {n - 1, (u_char*)name},
            NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
            ShmLiveDataZoneParamSetter,
            NGX_HTTP_MAIN_CONF_OFFSET,
            0,
            nullptr};
    }

    char* ShmCacheZoneParamSetter(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) try {
        (void)cmd;
        (void)conf;
        Y_ENSURE(cf);
        Y_ENSURE(cf->args->nelts == 6);

        ngx_str_t& name = ((ngx_str_t*)cf->args->elts)[1];
        ngx_str_t& sizeStr = ((ngx_str_t*)cf->args->elts)[2];
        ngx_str_t& readyTimeoutStr = ((ngx_str_t*)cf->args->elts)[3];
        ngx_str_t& inProgressTimeoutStr = ((ngx_str_t*)cf->args->elts)[4];
        ngx_str_t& maxWaitTimeoutStr = ((ngx_str_t*)cf->args->elts)[5];

        const TShmCache::TSettings settings{
            .ReadyTimeout = FromString<ngx_msec_t>((char*)readyTimeoutStr.data, readyTimeoutStr.len),
            .InProgressTimeout = FromString<ngx_msec_t>((char*)inProgressTimeoutStr.data, inProgressTimeoutStr.len),
            .MaxWaitTimeout = FromString<ngx_msec_t>((char*)maxWaitTimeoutStr.data, maxWaitTimeoutStr.len),
        };

        ssize_t size = ngx_parse_size(&sizeStr);
        Y_ENSURE(size != NGX_ERROR);

        TShmZone<TShmCache> zone = TShmZone<TShmCache>::CreateNew(name, size, cf);

        zone.Data().SetSettings(settings);

        return NGX_CONF_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "c++ exception [[ %s ]] ", e.what());
        return (char*)NGX_CONF_ERROR;
    }

    template <int n>
    constexpr ngx_command_t SetShmCacheZoneParam(const char (&name)[n]) {
        return {
            {n - 1, (u_char*)name},
            NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE5,
            ShmCacheZoneParamSetter,
            NGX_HTTP_MAIN_CONF_OFFSET,
            0,
            nullptr};
    }

    const ngx_command_t PackagerCommands[] = {
        // >> temp
        SetHandler<NTemp::TTestWorker>("packager_test"),
        SetHandler<NTemp::TTestReadWorker>("packager_test_read"),
        SetParam("packager_test_uri", &TLocationConfig::TestURI),
        SetParam("packager_test_sr_count", &TLocationConfig::TestSubrequestsCount),

        SetShmCacheZoneParam("packager_test_shm_zone"),
        SetParam("packager_test_shm_cache_zone", &TLocationConfig::TestShmCacheZone),
        SetHandler<NTemp::TTestCacheWorker>("packager_test_cache"),

        SetHandler<NTemp::TMP4VodTestWorker>("packager_test_mp4_vod"),
        SetHandler<NTemp::TMpegTsVodTestWorker>("packager_test_mpegts_vod"),

        SetHandler<NTemp::TTestLiveManagerSubscribeWorker>("packager_test_live_manager_subscribe"),

        SetHandler<NTemp::TLiveTestWorker>("packager_test_live"),

        SetHandler<NTemp::TTestTimedMetaWorker>("packager_test_timed_meta"),
        // temp <<

        SetHandler<TMusicWorker>("packager_music"),

        SetShmCacheZoneParam("packager_declare_shm_cache_zone"),

        SetParam("packager_chunk_duration_milliseconds", &TLocationConfig::ChunkDuration),
        SetParam("packager_moov_scan_block_size", &TLocationConfig::MoovScanBlockSize),
        SetParam("packager_moov_shm_cache_zone", &TLocationConfig::MoovShmCacheZone),
        SetParam("packager_description_shm_cache_zone", &TLocationConfig::DescriptionShmCacheZone),
        SetParam("packager_max_media_data_subrequest_size", &TLocationConfig::MaxMediaDataSubrequestSize),
        SetParam("packager_subrequest_clear_variables", &TLocationConfig::SubrequestClearVariables),

        SetParam("packager_subrequest_headers_drop", &TLocationConfig::SubrequestHeadersDrop),

        SetHandler<TVodWorker>("packager_vod"),
        SetParam("packager_url", &TLocationConfig::URI),
        SetParam("packager_meta_location", &TLocationConfig::MetaLocation),
        SetParam("packager_content_location", &TLocationConfig::ContentLocation),
        SetParam("packager_prerolls", &TLocationConfig::Prerolls),
        SetParam("packager_prerolls_clip_id", &TLocationConfig::PrerollsClipId),

        SetParam("packager_sign_secret", &TLocationConfig::SignSecret),
        SetParam("packager_encrypt_secret", &TLocationConfig::EncryptSecret),

        SetParam("packager_drm_enable", &TLocationConfig::DrmEnable),
        SetParam("packager_drm_request_uri", &TLocationConfig::DrmRequestUri),
        SetParam("packager_drm_mp4_protection_scheme", &TLocationConfig::DrmMp4ProtectionScheme),
        SetParam("packager_drm_whole_segment_aes_128", &TLocationConfig::DrmWholeSegmentAes128),

        SetParam("packager_allow_drm_content_unencrypted", &TLocationConfig::AllowDrmContentUnencrypted),

        SetShmLiveDataZoneParam("packager_live_data_shm_zone"),
        SetParam("packager_live_manager_tick_period", &TRequestContext::TMainConfig::EternalTimerPeriod),
        SetParam("packager_live_manager_short_tick_period", &TRequestContext::TMainConfig::EternalTimerShortPeriod),
        SetParam("packager_live_manager_shm_store_ttl", &TLiveManager::TSettings::ShmStoreTTL),
        SetParam("packager_live_manager_max_grpc_work_time", &TLiveManager::TSettings::MaxGrpcWorkTime),
        SetParam("packager_live_manager_max_health_tick_delay", &TLiveManager::TSettings::MaxHealthTickDelay),
        SetParam("packager_live_manager_old_stream_timeout", &TLiveManager::TSettings::OldStreamTimeout),
        SetParam("packager_live_manager_grpc_reconnect_timeout", &TLiveManager::TSettings::GrpcReconnectTimeout),
        SetParam("packager_live_manager_grpc_arg_initial_reconnect_backoff_ms", &TLiveManager::TSettings::GrpcArgInitialReconnectBackoffMs),
        SetParam("packager_live_manager_grpc_arg_max_reconnect_backoff_ms", &TLiveManager::TSettings::GrpcArgMaxReconnectBackoffMs),
        SetParam("packager_live_manager_grpc_arg_keepalive_timeout_ms", &TLiveManager::TSettings::GrpcArgKeepaliveTimeoutMs),
        SetParam("packager_live_manager_grpc_arg_keepalive_time_ms", &TLiveManager::TSettings::GrpcArgKeepaliveTimeMs),
        SetParam("packager_live_manager_grpc_arg_http2_min_sent_ping_interval_without_data_ms", &TLiveManager::TSettings::GrpcArgHttp2MinSentPingIntervalWithoutDataMs),
        SetParam("packager_live_manager_latency_log_period", &TLiveManager::TSettings::LatencyLogPeriod),

        SetParam("packager_live_manager_grpc_targets", &TLiveManager::TSettings::YouberTargets), // deprecated
        SetParam("packager_live_manager_youber_targets", &TLiveManager::TSettings::YouberTargets),
        SetParam("packager_live_manager_ypsd_target", &TLiveManager::TSettings::YPSDTarget),
        SetParam("packager_live_manager_ypsd_clusters", &TLiveManager::TSettings::YPSDClusters),
        SetParam("packager_live_manager_ypsd_request_period", &TLiveManager::TSettings::GrpcYPSDRequestPeriod),
        SetParam("packager_live_manager_ypsd_resolve_timeout", &TLiveManager::TSettings::YPSDResolveTimeout),
        SetParam("packager_live_manager_ypsd_resolve_cache_buffer_size", &TLiveManager::TSettings::YPSDResolveCacheBufferSize),
        SetParam("packager_live_manager_grpc_target_resolve", &TLiveManager::TSettings::YouberResolveFromConfig),

        SetParam("packager_live_manager_access", &TLocationConfig::LiveManagerAccess),

        SetParam("packager_max_media_ts_gap_milliseconds", &TLocationConfig::MaxMediaTsGap),
        SetParam("packager_transcoders_live_location_low_latency", &TLocationConfig::TranscodersLiveLocationCacheFollow), // deprecated
        SetParam("packager_transcoders_live_location_cache_follow", &TLocationConfig::TranscodersLiveLocationCacheFollow),
        SetParam("packager_transcoders_live_location_cache_lock", &TLocationConfig::TranscodersLiveLocationCacheLock),
        SetParam("packager_live_video_track_name", &TLocationConfig::LiveVideoTrackName),
        SetParam("packager_live_audio_track_name", &TLocationConfig::LiveAudioTrackName),
        SetParam("packager_live_cmaf_flag", &TLocationConfig::LiveCmafFlag),
        SetParam("packager_live_future_chunk_limit", &TLocationConfig::LiveFutureChunkLimit),

        SetHandler<TLiveWorker>("packager_live"),

        SetHandler<TKalturaImitationWorker>("packager_kaltura_imitation_worker"),
        SetParam("packager_playlist_json_uri", &TLocationConfig::PlaylistJsonUri),
        SetParam("packager_playlist_json_args", &TLocationConfig::PlaylistJsonArgs),

        SetParam("packager_subtitles_ttml_style", &TLocationConfig::SubtitlesTTMLStyle),
        SetParam("packager_subtitles_ttml_region", &TLocationConfig::SubtitlesTTMLRegion),

        DeletedParam("packager_live_manager_full_state_write_period_in_ticks"),

        ngx_null_command};

    const ngx_http_module_t PackagerContextHeaderFilter = {
        NULL,                                           // preconfiguration
        TRequestContext::PostConfigurationHeaderFilter, // postconfiguration

        NULL, // create main configuration
        NULL, // init main configuration

        NULL, // create server configuration
        NULL, // merge server configuration

        NULL, // create location configuration
        NULL  // merge location configuration
    };

    const ngx_http_module_t PackagerContextBodyFilter = {
        TRequestContext::PreConfigurationBodyFilter,  // preconfiguration
        TRequestContext::PostConfigurationBodyFilter, // postconfiguration

        TRequestContext::CreateMainConfig, // create main configuration
        NULL,                              // init main configuration

        NULL, // create server configuration
        NULL, // merge server configuration

        TRequestContext::CreateLocationConfig, // create location configuration
        TRequestContext::MergeLocationConfig   // merge location configuration
    };

    extern "C" ngx_module_t ngx_http_strm_packager_module_header_filter = {
        NGX_MODULE_V1,
        (void*)&PackagerContextHeaderFilter, // module context
        NULL,                                // module directives
        NGX_HTTP_MODULE,                     // module type
        NULL,                                // init master
        NULL,                                // init module
        NULL,                                // init process
        NULL,                                // init thread
        NULL,                                // exit thread
        NULL,                                // exit process
        NULL,                                // exit master
        NGX_MODULE_V1_PADDING};

    extern "C" ngx_module_t ngx_http_strm_packager_module_body_filter = {
        NGX_MODULE_V1,
        (void*)&PackagerContextBodyFilter, // module context
        (ngx_command_t*)PackagerCommands,  // module directives
        NGX_HTTP_MODULE,                   // module type
        NULL,                              // init master
        NULL,                              // init module
        &TRequestContext::OnInitProcess,   // init process
        NULL,                              // init thread
        NULL,                              // exit thread
        NULL,                              // exit process
        NULL,                              // exit master
        NGX_MODULE_V1_PADDING};

}
