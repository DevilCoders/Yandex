#pragma once

extern "C" {
#include <ngx_http.h>
}

#include <functional>

namespace NStrm::NPackager {
    class TNgxTimer {
    public:
        using TCallback = std::function<void()>;

        TNgxTimer(ngx_http_request_t* request);
        ~TNgxTimer();
        void ResetCallback(TCallback callback);
        void ResetTime(ngx_msec_t t);

    private:
        static void EventHandler(ngx_event_t* event);

        ngx_event_t Event;
        TCallback Callback;
    };
}
