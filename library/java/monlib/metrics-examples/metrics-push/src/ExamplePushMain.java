package ru.yandex.monlib.metrics.example.push;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.TimeUnit;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.config.LoggerConfig;

import ru.yandex.monlib.metrics.CompositeMetricSupplier;
import ru.yandex.monlib.metrics.JvmGc;
import ru.yandex.monlib.metrics.JvmMemory;
import ru.yandex.monlib.metrics.JvmRuntime;
import ru.yandex.monlib.metrics.JvmThreads;
import ru.yandex.monlib.metrics.example.push.client.MetricsPushClient;
import ru.yandex.monlib.metrics.example.push.metrics.HttpClientMetrics;
import ru.yandex.monlib.metrics.example.push.services.SeriesBotService;
import ru.yandex.monlib.metrics.example.services.series.metrics.SeriesMetrics;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

/**
 * Example of Pushing metrics.
 * <p> Push client every 15 seconds pushes metrics
 * {@link ru.yandex.monlib.metrics.example.push.client.MetricsPushClient}
 *
 * <p> For simple example
 * {@link HttpClientMetrics}
 *
 * <p> For advanced example with MetricSupplier and aggregation
 * {@link ru.yandex.monlib.metrics.example.services.series.metrics.SeriesMetrics}
 *
 * <p> Some job for metric changes
 * {@link ru.yandex.monlib.metrics.example.push.services.SeriesBotService}
 *
 * @author Alexey Trushkin
 */
public class ExamplePushMain {

    public void start() throws IOException {
        // configs
        var configs = getConfigs();
        var jsonPath = getPropertyOrThrow("jsonPath");

        // config app metrics
        var metricRegistry = MetricRegistry.root();
        registerJvmMetrics(metricRegistry);

        var seriesMetrics = new SeriesMetrics();
        var metricSupplier = new CompositeMetricSupplier(List.of(metricRegistry, seriesMetrics));

        // schedule task, which pushes metrics
        MetricsPushClient.create(metricRegistry).schedulePush(
                configs,
                metricSupplier, 15, TimeUnit.SECONDS);

        // run series bot for metric changes
        new SeriesBotService(seriesMetrics, jsonPath).start();
    }

    private void registerJvmMetrics(MetricRegistry registry) {
        JvmRuntime.addMetrics(registry);
        JvmGc.addMetrics(registry);
        JvmThreads.addMetrics(registry);
        JvmMemory.addMetrics(registry);
    }

    private PushConfigs getConfigs() {
        var project = getPropertyOrThrow("project");
        var cluster = getPropertyOrThrow("cluster");
        var service = getPropertyOrThrow("service");
        var url = getPropertyOrThrow("url");
        var token = getPropertyOrThrow("oAuthToken");
        return new PushConfigs(project, cluster, service, url, token);
    }

    private String getPropertyOrThrow(String key) {
        var value = System.getProperty(key, "");
        if (value.isBlank()) {
            throw new IllegalArgumentException(key + " must be specified");
        }
        return value;
    }

    private static void configLogger() {
        LoggerConfig rootLogger = LoggerContext.getContext(false).getConfiguration().getRootLogger();
        rootLogger.setAdditive(false);
        rootLogger.setLevel(Level.DEBUG);
    }

    public static void main(String[] args) {
        configLogger();
        try {
            new ExamplePushMain().start();
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

}
