#pragma once

extern "C" {
#include <ngx_http.h>
}

#include <util/generic/maybe.h>

namespace NStrm::NPackager {
    using TComplexValue = ngx_http_complex_value_t;

    struct TMetrics {
        // metrics support only float fields
        TMaybe<float> IsCmaf;

        // Live metrics
        TMaybe<float> LiveCreateLiveSourceDelay;
        TMaybe<float> LiveIsLowLatencyMode;
        TMaybe<float> LiveFirstFragmentDelay;
        TMaybe<float> LiveFirstFragmentDuration;
        TMaybe<float> LiveFirstFragmentLatency;
        TMaybe<float> LiveLastFragmentDelay;
        TMaybe<float> LiveLastFragmentDuration;
        TMaybe<float> LiveLastFragmentLatency;
        TMaybe<float> LiveWriteServerTimeUuidBox;
    };

    struct TVariables {
        TMetrics Metrics;
    };
}
