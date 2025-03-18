package ru.yandex.ci.metrics;

import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Counter;
import io.prometheus.client.Histogram;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import retrofit2.Call;
import retrofit2.Response;

import ru.yandex.ci.client.base.http.CallListener;
import ru.yandex.ci.client.base.http.CallsMonitor;
import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.RetrofitLoggingCalls;

@Slf4j
@RequiredArgsConstructor
public class PrometheusCallsMonitorSource implements CallsMonitorSource  {

    private static final String NS_HTTP = "http";
    private static final String SUBSYSTEM_CLIENT = "client";

    private static final String LABEL_CLIENT_TYPE = "client_type";
    private static final String LABEL_CLIENT_NAME = "client_name";
    private static final String LABEL_RET_CODE = "ret_code";

    private final Histogram latencyHistogram;
    private final Counter responseCounter;

    public PrometheusCallsMonitorSource(CollectorRegistry registry, double[] buckets) {
        this.latencyHistogram = Histogram.build()
                .namespace(NS_HTTP)
                .subsystem(SUBSYSTEM_CLIENT)
                .name("client_latency_seconds")
                .help("Latency for outgoing HTTP calls")
                .labelNames(LABEL_CLIENT_TYPE, LABEL_CLIENT_NAME) // TODO: add method, add url
                .buckets(buckets)
                .register(registry);

        this.responseCounter = Counter.build()
                .namespace(NS_HTTP)
                .subsystem(SUBSYSTEM_CLIENT)
                .name("client_response_code")
                .help("Response codes for outgoing HTTP calls")
                .labelNames(LABEL_CLIENT_TYPE, LABEL_CLIENT_NAME, LABEL_RET_CODE)
                .register(registry);
    }

    @Override
    public CallsMonitor<Call<?>, Response<?>> buildRetrofitMonitor(String clientName) {
        log.info("Register calls monitor for retrofit, [{}]", clientName);
        return RetrofitLoggingCalls.create(clientName, httpClientCallListener("retrofit", clientName));
    }

    //

    private CallListener httpClientCallListener(String clientType, String clientName) {
        var histogram = latencyHistogram.labels(clientType, clientName);
        return (totalTimeMillis, retCode) -> {
            double timeSeconds = totalTimeMillis / 1000.0;
            histogram.observe(timeSeconds);
            responseCounter.labels(clientType, clientName, String.valueOf(retCode)).inc();
        };
    }
}
