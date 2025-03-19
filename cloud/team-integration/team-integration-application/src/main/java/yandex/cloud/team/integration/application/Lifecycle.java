package yandex.cloud.team.integration.application;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.time.Duration;

import javax.inject.Inject;

import io.grpc.Server;
import io.prometheus.client.hotspot.DefaultExports;
import yandex.cloud.application.ServiceApplication;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.health.http.ManagedHealthCheck;
import yandex.cloud.health.http.ManagedHealthCheckConfig;
import yandex.cloud.iam.repository.Bootstrap;
import yandex.cloud.metrics.MetricReporter;
import yandex.cloud.task.TaskProcessorLifecycle;
import yandex.cloud.task.model.Task;
import yandex.cloud.team.integration.config.ExperimentalConfig;

public class Lifecycle implements ServiceApplication.Lifecycle {

    @Inject
    private static Bootstrap bootstrap;

    @Inject
    private static HttpServer httpServer;

    @Inject
    private static Server grpcServer;

    @Inject
    private static MetricReporter metricReporter;

    @Inject
    private static TaskProcessorLifecycle taskProcessor;

    @Inject
    private static SystemAccountService systemAccountService;

    @Inject
    private static ManagedHealthCheckConfig healthCheckConfig;

    @Inject
    private static ManagedHealthCheck healthCheck;

    @Inject
    private static ExperimentalConfig experimentalConfig;

    public Lifecycle() {
        bootstrap.run();
    }

    @Override
    public void start() {
        Task.Action.enabled = () -> true;
        Task.Action.used = () -> experimentalConfig != null && experimentalConfig.isUseTaskProcessorActionIndex();

        if (healthCheckConfig != null) {
            healthCheck = new ManagedHealthCheck(healthCheckConfig);
            healthCheck.startServer();
        }

        DefaultExports.initialize();
        systemAccountService.startReload();
        metricReporter.start();
        taskProcessor.start();
        httpServer.start();
        try {
            grpcServer.start();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }

        if (healthCheck != null) {
            healthCheck.enable();
        }
    }

    @Override
    public void preShutdown() {
        if (healthCheck != null) {
            healthCheck.disable();
        }
    }

    @Override
    public Duration getShutdownDelay() {
        return healthCheckConfig != null ? healthCheckConfig.getShutdownDelay() : null;
    }

    @Override
    public void stop() {
        grpcServer.shutdown();
        httpServer.stop();
        taskProcessor.shutdown();
        metricReporter.stop();
        systemAccountService.shutdownReload();

        if (healthCheck != null) {
            healthCheck.stopServer();
            healthCheck = null;
        }
    }

}
