package ru.yandex.monlib.metrics.example.push.metrics;

import java.util.concurrent.ConcurrentHashMap;

import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

/**
 * API metrics for http client method + endpoint:
 * <p>
 * <p> RATE http.client.call.started - endpoint rps
 * <p> RATE http.client.call.completed.error - endpoint rps ended with error
 * <p> RATE http.client.call.completed.ok - endpoint rps ended without error
 * <p> HISTOGRAM http.client.call.duration.ms - histogram of request handling duration
 * <p> RATE http.client.call.total - response count with some http code
 * <p> IGAUGE http.client.call.inFlight - requests in flight
 *
 * @author Alexey Trushkin
 */
public class HttpClientMetrics {

    private final ConcurrentHashMap<String, InnerMetric> metrics;
    private final MetricRegistry metricRegistry;

    public HttpClientMetrics(MetricRegistry metricRegistry) {
        metrics = new ConcurrentHashMap<>();
        // use root registry for all metrics
        this.metricRegistry = metricRegistry;
    }

    public void hitRequest(String url, int code, String methodName, long durationMs) {
        var metric = getMetric(url, methodName);
        metric.completedOk.inc();
        metric.responseTimeMillis.record(durationMs);
        metric.getRequestsTotalMetric(code).inc();
    }

    public void hitRequestFailed(String url, int code, String methodName, long durationMs) {
        var metric = getMetric(url, methodName);
        metric.completedError.inc();
        metric.responseTimeMillis.record(durationMs);
        metric.getRequestsTotalMetric(code).inc();
    }

    public void requestStarted(String url, String methodName) {
        var metric = getMetric(url, methodName);
        metric.started.inc();
    }

    private InnerMetric getMetric(String url, String methodName) {
        return metrics.computeIfAbsent(url + methodName,
                key -> new InnerMetric(url, methodName));
    }

    private class InnerMetric {

        private final Rate started;
        private final Rate completedError;
        private final Rate completedOk;
        private final Histogram responseTimeMillis;
        private final Labels commonLabels;

        private InnerMetric(String url, String methodName) {
            commonLabels = Labels.of("endpoint", url, "method", methodName);
            started = metricRegistry.rate("http.client.call.started", commonLabels);
            completedError = metricRegistry.rate("http.client.call.completed.error", commonLabels);
            completedOk = metricRegistry.rate("http.client.call.completed.ok", commonLabels);
            metricRegistry.lazyGaugeInt64("http.client.call.inFlight", commonLabels,
                    () -> started.get() - completedOk.get() - completedError.get());
            responseTimeMillis = metricRegistry.histogramCounter("http.client.call.duration.ms", commonLabels,
                    Histograms.exponential(13, 2, 30));
        }

        private Rate getRequestsTotalMetric(int status) {
            Labels labels = commonLabels.toBuilder()
                    .add("code", Integer.toString(status))
                    .build();
            return metricRegistry.rate("http.client.call.total", labels);
        }
    }
}
