package ru.yandex.monlib.metrics.http;

import java.util.concurrent.ConcurrentHashMap;

import javax.annotation.ParametersAreNonnullByDefault;

import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

/**
 * @author Oleg Baryshnikov
 */
@ParametersAreNonnullByDefault
public class HttpStatsMetrics {

    private final MetricRegistry registry;
    private final ConcurrentHashMap<String, EndpointMetrics> metricsByEndpoint;

    public HttpStatsMetrics(MetricRegistry registry) {
        this.registry = registry;
        this.metricsByEndpoint = new ConcurrentHashMap<>();
    }

    public EndpointMetrics getEndpointsMetrics(String endpoint, String method) {
        String mapKey = endpoint + ":" + method;
        return metricsByEndpoint.computeIfAbsent(mapKey, key -> EndpointMetrics.of(endpoint, method, registry));
    }

    public static class EndpointMetrics {
        private final MetricRegistry registry;
        private final Labels commonLabels;

        private final Rate started;
        private final Rate completed;
        private final Histogram responseDurationMillis;

        private EndpointMetrics(MetricRegistry registry, Labels commonLabels) {
            this.registry = registry;
            this.commonLabels = commonLabels;
            started = registry.rate("http.server.requests.started", commonLabels);
            completed = registry.rate("http.server.requests.completed", commonLabels);
            registry.lazyGaugeInt64("http.server.requests.inFlight", commonLabels, this::getInFlight);
            responseDurationMillis =
                    registry.histogramRate("http.server.requests.elapsedTimeMs", commonLabels,
                            Histograms.exponential(18, 2, 16));
        }

        private static EndpointMetrics of(String endpoint, String method, MetricRegistry registry) {
            Labels commonLabels = Labels.of("endpoint", endpoint, "method", method);
            return new EndpointMetrics(registry, commonLabels);
        }

        public void callStarted() {
            started.inc();
        }

        public void callCompleted(int statusCode, long durationMillis) {
            completed.inc();
            responseDurationMillis.record(durationMillis);
            getRequestsTotalMetric(statusCode).inc();
        }

        private Rate getRequestsTotalMetric(int status) {
            Labels labels = commonLabels.toBuilder()
                    .add("code", Integer.toString(status))
                    .build();
            return registry.rate("http.server.requests.status", labels);
        }

        private long getInFlight() {
            return started.get() - completed.get();
        }
    }
}
