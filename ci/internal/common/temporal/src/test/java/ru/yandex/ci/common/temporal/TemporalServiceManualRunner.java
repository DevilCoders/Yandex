package ru.yandex.ci.common.temporal;

import java.time.Duration;
import java.util.concurrent.TimeUnit;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.logging.LoggingMeterRegistry;

import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;
import ru.yandex.ci.common.temporal.heartbeat.TemporalWorkerHeartbeatService;
import ru.yandex.ci.common.temporal.monitoring.LoggingMonitoringService;
import ru.yandex.ci.common.temporal.workflow.logging.LoggingActivity;
import ru.yandex.ci.common.temporal.workflow.logging.LoggingWorkflow;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestWorkflow;


/***
 * Специальный класс для локального тестирования воркфлоу поверх настоящего темпорала.
 */
public class TemporalServiceManualRunner {

    private static final MeterRegistry METER_REGISTRY = new LoggingMeterRegistry();
    public static final String QUEUE = "1";

    private TemporalServiceManualRunner() {
    }

    public static TemporalService createTemporalService(String namespace) {
        System.setProperty("TEMPORAL_DEBUG", "true");


        return TemporalConfigurationUtil.createTemporalService(
                "dns:///ci-temporal-frontend-testing.in.yandex.net:7233",
                namespace,
                null,
                METER_REGISTRY,
                "http://ci-temporal-testing.yandex.net/"
        );
    }

    public static void main(String[] args) throws Exception {
        var namespace = System.getProperty("user.name") + "-dev";
        var temporalService = createTemporalService(namespace);

        var workerFactory = TemporalConfigurationUtil.createWorkerFactoryBuilder(temporalService)
                .monitoringService(new LoggingMonitoringService())
                .heartbeatService(new TemporalWorkerHeartbeatService(Duration.ofMinutes(1), 2, METER_REGISTRY))
                .worker(TemporalConfigurationUtil.createWorker(QUEUE)
                        .workflowImplementationTypes(LoggingWorkflow.class)
                        .activitiesImplementations(new LoggingActivity()))
                .build();

        workerFactory.start();
        temporalService.startDeduplicated(SimpleTestWorkflow.class, wf -> wf::run, SimpleTestId.ofCurrentDate(), QUEUE);

        Runtime.getRuntime().addShutdownHook(
                new Thread(workerFactory::shutdown)
        );

        while (true) {
            TimeUnit.SECONDS.sleep(1);
        }
    }

}
