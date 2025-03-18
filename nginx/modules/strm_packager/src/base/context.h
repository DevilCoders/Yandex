#pragma once

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <nginx/modules/strm_packager/src/base/live_manager.h>
#include <nginx/modules/strm_packager/src/base/pool_util.h>
#include <nginx/modules/strm_packager/src/base/timer.h>
#include <nginx/modules/strm_packager/src/base/types.h>

#include <util/generic/buffer.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <functional>

namespace NStrm::NPackager {
    class TRequestWorker;
    class ISubrequestWorker;
    class TLocationConfig;

    class TRequestContext {
        friend class TNgxPoolUtil<TRequestContext>;
        friend class TRequestWorker;

    public:
        template <typename TWorker>
        static ngx_int_t RequestHandler(ngx_http_request_t* request);

        static ngx_int_t PreConfigurationBodyFilter(ngx_conf_t* cf);
        static ngx_int_t PostConfigurationBodyFilter(ngx_conf_t* cf);
        static ngx_int_t PostConfigurationHeaderFilter(ngx_conf_t* cf);

        static void* CreateMainConfig(ngx_conf_t* cf);

        static ngx_int_t OnInitProcess(ngx_cycle_t* cycle);

        static void* CreateLocationConfig(ngx_conf_t* cf);
        static char* MergeLocationConfig(ngx_conf_t* cf, void* parent, void* child);

        static void ExceptionLogAndFinalizeRequest(const std::exception_ptr e, ngx_http_request_t* request);

        static TVector<ngx_http_variable_t> Vars;

        template <typename TParam>
        static ngx_int_t GetVariableHandler(ngx_http_request_t* request, ngx_http_variable_value_t* var, uintptr_t data);

    public:
        class TMainConfig {
        public:
            TMainConfig() {
                std::memset(&EternalTimerEvent, 0, sizeof(EternalTimerEvent));
            }

            TSet<TLocationConfig*> Locations;

            ngx_event_t EternalTimerEvent;
            ngx_msec_t EternalTimerShortPeriod = 10;
            ngx_msec_t EternalTimerPeriod = 50;

            TMaybe<TShmZone<TLiveManager::TShmLiveData>> ShmLiveData;
            TMaybe<TLiveManager> LiveManager;
            TLiveManager::TSettings LiveManagerSettings;
        };

    private:
        class TPostSubrequestData {
        public:
            TPostSubrequestData(ISubrequestWorker& worker)
                : Worker(worker)
                , Headers(false)
                , Finished(false)
                , GotLastInChain(false)
            {
            }

            ISubrequestWorker& Worker;
            bool Headers;
            bool Finished;
            bool GotLastInChain;

            ui64 DataSize;
            ui64 DataSizeOnLastInChain;
        };

        explicit TRequestContext(ngx_http_request_t* request);
        ~TRequestContext();

        static TRequestContext* RawGet(ngx_http_request_t* request);
        static TRequestContext* Get(ngx_http_request_t* request);
        static void SetContext(ngx_http_request_t* request, TRequestContext* context);
        static bool IsSubrequest(ngx_http_request_t* request);
        static ngx_int_t SubrequestFinishedCallback(ngx_http_request_t* subrequest, void* data, ngx_int_t returnCode);

        static ngx_int_t OutputBodyFilter(ngx_http_request_t* request, ngx_chain_t* chain);
        static ngx_int_t OutputHeaderFilter(ngx_http_request_t* request);

        static void EternalTimerEventHandler(ngx_event_t* event);

        static ngx_http_output_body_filter_pt NextOutputBodyFilter;
        static ngx_http_output_header_filter_pt NextOutputHeaderFilter;

        // ssi module is not supposed to be enabled for main request location
        //   so it is safe to use NGX_HTTP_SSI_BUFFERED flag to mark main request buffered
        static const int BufferedFlag = NGX_HTTP_SSI_BUFFERED;

        void StartFinalize();

        ngx_http_request_t* const Request;
        THolder<TRequestWorker> Worker;
        bool Aborting;
        bool Finished;
        bool InRequestHandler;

        ui32 RunningSubrequests;

        TVariables Variables;

        ngx_chain_t* FlushChain;
        ngx_chain_t* FinishChain;

        TNgxTimer FinalizeTimer;
        TMaybe<int> FinalizeStatus;
    };
}
