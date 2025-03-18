package ru.yandex.monlib.metrics.example.push.client;

import java.io.ByteArrayOutputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.asynchttpclient.AsyncHttpClient;
import org.asynchttpclient.DefaultAsyncHttpClient;
import org.asynchttpclient.RequestBuilder;
import org.asynchttpclient.util.HttpConstants;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.monlib.metrics.MetricSupplier;
import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.encode.MetricFormat;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;
import ru.yandex.monlib.metrics.example.push.PushConfigs;
import ru.yandex.monlib.metrics.example.push.consumer.PushMetricStatefulConsumer;
import ru.yandex.monlib.metrics.example.push.metrics.HttpClientMetrics;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static ru.yandex.monlib.metrics.encode.MetricEncoderFactory.createEncoder;

/**
 * Metrics push client.
 * Push metrics with fixed grid timestamp
 *
 * @author Alexey Trushkin
 */
public class MetricsPushClient implements AutoCloseable {
    private static final Logger logger = LoggerFactory.getLogger(MetricsPushClient.class);
    private final AsyncHttpClient httpClient;
    private final ScheduledExecutorService executorService;
    private final HttpClientMetrics metrics;

    private MetricsPushClient(AsyncHttpClient httpClient, ScheduledExecutorService executorService,
                              MetricRegistry metricRegistry) {
        this.httpClient = httpClient;
        this.executorService = executorService;
        metrics = new HttpClientMetrics(metricRegistry);
    }

    public static MetricsPushClient create(MetricRegistry metricRegistry) {
        AsyncHttpClient httpClient = new DefaultAsyncHttpClient();
        ScheduledExecutorService executorService =
                Executors.newSingleThreadScheduledExecutor(new InnerThreadFactory());

        return new MetricsPushClient(httpClient, executorService, metricRegistry);
    }

    public void schedulePush(PushConfigs configs, MetricSupplier supplier, long interval, TimeUnit unit) {
        executorService.scheduleWithFixedDelay(new PushTask(configs, supplier, unit.toMillis(interval)), 1, interval, unit);
    }

    @Override
    public void close() throws Exception {
        executorService.shutdownNow();
        httpClient.close();
    }

    private class PushTask implements Runnable {
        // always use SPACK as default format
        private final MetricFormat format = MetricFormat.SPACK;
        private final PushConfigs configs;
        private final MetricSupplier supplier;
        private final long intervalMillis;
        private final Map<Labels, Long> rateMetricsState;

        public PushTask(PushConfigs configs, MetricSupplier supplier, long intervalMillis) {
            this.configs = configs;
            this.supplier = supplier;
            this.intervalMillis = intervalMillis;
            rateMetricsState = new HashMap<>();
        }

        @Override
        public void run() {
            metrics.requestStarted(configs.url, HttpConstants.Methods.POST);
            long start = System.nanoTime();
            httpClient.executeRequest(new RequestBuilder(HttpConstants.Methods.POST)
                    .setUrl(configs.url
                            + "?project=" + configs.project
                            + "&cluster=" + configs.cluster
                            + "&service=" + configs.service)
                    .setBody(encode())
                    .addHeader("content-type", format.contentType())
                    .addHeader("authorization", "OAuth " + configs.token)
                    .build())
                    .toCompletableFuture()
                    .whenComplete((response, throwable) -> {
                        int code = response.getStatusCode();
                        long duration = TimeUnit.NANOSECONDS.toMillis(System.nanoTime() - start);
                        if (throwable != null) {
                            logger.warn("failed push to shard {} : {}", configs.getShardString(), throwable);
                            metrics.hitRequestFailed(configs.url, code, HttpConstants.Methods.POST, duration);
                            return;
                        }

                        if (code < 200 || code > 299) {
                            logger.warn("failed push to shard {}: {}", configs.getShardString(), response);
                            metrics.hitRequestFailed(configs.url, code, HttpConstants.Methods.POST, duration);
                            return;
                        }

                        metrics.hitRequest(configs.url, code, HttpConstants.Methods.POST, duration);
                    });
        }

        private byte[] encode() {
            // calculate grid timestamp
            long now = System.currentTimeMillis();
            long tsMillisByGrid = now - (now % intervalMillis);

            ByteArrayOutputStream out = new ByteArrayOutputStream(8 << 10); // 8 KiB
            try (MetricEncoder encoder = createEncoder(out, format, CompressionAlg.LZ4)) {
                var consumer = new PushMetricStatefulConsumer(encoder, rateMetricsState, (int) (intervalMillis / 1000));
                consumer.onStreamBegin(supplier.estimateCount());
                consumer.onCommonTime(tsMillisByGrid);
                supplier.append(0, Labels.empty(), consumer);
                consumer.onStreamEnd();
            } catch (Exception e) {
                throw new IllegalStateException("cannot encode sensors", e);
            }
            return out.toByteArray();
        }
    }

    private static class InnerThreadFactory implements ThreadFactory {
        @Override
        public Thread newThread(Runnable r) {
            Thread thread = new Thread(r);
            thread.setDaemon(true);
            thread.setName(MetricsPushClient.class.getSimpleName());
            return thread;
        }
    }
}
