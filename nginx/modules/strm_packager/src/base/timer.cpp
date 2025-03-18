#include <nginx/modules/strm_packager/src/base/context.h>
#include <nginx/modules/strm_packager/src/base/fatal_exception.h>
#include <nginx/modules/strm_packager/src/base/timer.h>

#include <util/generic/yexception.h>

namespace NStrm::NPackager {
    TNgxTimer::TNgxTimer(ngx_http_request_t* request) {
        Y_ENSURE(request);

        std::memset(&Event, 0, sizeof(Event));
        Event.data = request->connection;
        Event.handler = EventHandler;
        Event.log = request->connection->log;
    };

    TNgxTimer::~TNgxTimer() {
        if (Event.timer_set) {
            ngx_del_timer(&Event);
        }
    }

    void TNgxTimer::ResetCallback(TCallback callback) {
        Callback = callback;
    }

    void TNgxTimer::ResetTime(ngx_msec_t t) {
        ngx_add_timer(&Event, t);
    }

    // static
    void TNgxTimer::EventHandler(ngx_event_t* event) {
        TNgxTimer& timer = *(TNgxTimer*)(((ui8*)event) - (size_t) & (((TNgxTimer*)nullptr)->Event));
        if (timer.Callback) {
            timer.Callback();
        }
    }
}
