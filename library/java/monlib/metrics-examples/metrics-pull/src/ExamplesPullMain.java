package ru.yandex.monlib.metrics.example.pull;


import java.time.Duration;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.config.LoggerConfig;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;
import org.springframework.http.server.reactive.HttpHandler;
import org.springframework.http.server.reactive.ReactorHttpHandlerAdapter;
import org.springframework.web.server.adapter.WebHttpHandlerBuilder;
import reactor.netty.http.server.HttpServer;

/**
 * Example of Pulling metrics.
 * <p> HttpServer provides pulling endpoint /api/v1/metrics for all metrics
 * {@link ru.yandex.monlib.metrics.example.pull.web.controllers.MetricsRestController}
 *
 * <p> For simple example
 * {@link ru.yandex.monlib.metrics.http.HttpStatsMetrics}
 *
 * <p> For advanced example with MetricSupplier and aggregation
 * {@link ru.yandex.monlib.metrics.example.services.series.metrics.SeriesMetrics}
 *
 * @author Alexey Trushkin
 */
public class ExamplesPullMain {

    public static void main(String[] args) {
        new ExamplesPullMain().start();
    }

    public void start() {
        configLogger();
        startServer();
    }

    private void startServer() {
        AnnotationConfigApplicationContext applicationContext = new AnnotationConfigApplicationContext(ExamplesPullMainContext.class);
        HttpHandler handler = WebHttpHandlerBuilder.applicationContext(applicationContext).build();
        ReactorHttpHandlerAdapter adapter = new ReactorHttpHandlerAdapter(handler);
        HttpServer.create()
                .port(getServerPort())
                .handle(adapter)
                .bindNow(Duration.ofSeconds(10))
                .onDispose()
                .block();
    }

    private void configLogger() {
        LoggerConfig rootLogger = LoggerContext.getContext(false).getConfiguration().getRootLogger();
        rootLogger.setAdditive(false);
        rootLogger.setLevel(Level.DEBUG);
    }

    private Integer getServerPort() {
        return Integer.getInteger("serverPort", 8082);
    }
}
