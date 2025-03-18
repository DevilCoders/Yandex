package ru.yandex.ci.common.temporal.config;

import com.uber.m3.tally.RootScopeBuilder;
import com.uber.m3.tally.Scope;
import com.uber.m3.tally.StatsReporter;
import com.uber.m3.util.Duration;
import io.micrometer.core.instrument.MeterRegistry;
import io.temporal.client.WorkflowClient;
import io.temporal.client.WorkflowClientOptions;
import io.temporal.common.converter.ByteArrayPayloadConverter;
import io.temporal.common.converter.DataConverter;
import io.temporal.common.converter.DefaultDataConverter;
import io.temporal.common.converter.JacksonJsonPayloadConverter;
import io.temporal.common.converter.NullPayloadConverter;
import io.temporal.common.converter.ProtobufJsonPayloadConverter;
import io.temporal.serviceclient.WorkflowServiceStubs;
import io.temporal.serviceclient.WorkflowServiceStubsOptions;

import yandex.cloud.util.Json;
import ru.yandex.ci.common.temporal.FixedTemporalMicrometerClientStatsReporter;
import ru.yandex.ci.common.temporal.TemporalMetricScoreWrapper;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.ydb.TemporalDb;

public class TemporalConfigurationUtil {

    private TemporalConfigurationUtil() {
    }

    public static TemporalService createTemporalService(
            String endpoint,
            String namespace,
            TemporalDb temporalDb,
            MeterRegistry meterRegistry,
            String uiBaseUrl
    ) {
        var client = createWorkflowClient(endpoint, namespace, meterRegistry);
        return new TemporalService(client, temporalDb, dataConverter(), uiBaseUrl);
    }

    public static WorkflowClient createWorkflowClient(
            String endpoint,
            String namespace,
            MeterRegistry meterRegistry
    ) {

        var options = WorkflowClientOptions.newBuilder()
                .setNamespace(namespace)
                .setDataConverter(dataConverter())
                .validateAndBuildWithDefaults();

        var stubs = createWorkflowServiceStubs(endpoint, meterRegistry);

        return WorkflowClient.newInstance(stubs, options);
    }

    public static DataConverter dataConverter() {
        return new DefaultDataConverter(
                new NullPayloadConverter(),
                new ByteArrayPayloadConverter(),
                new ProtobufJsonPayloadConverter(),
                new JacksonJsonPayloadConverter(Json.mapper)  //TODO move json to separate module and use CiJson
        );
    }

    private static WorkflowServiceStubs createWorkflowServiceStubs(String endpoint, MeterRegistry meterRegistry) {

        StatsReporter statsReporter = new FixedTemporalMicrometerClientStatsReporter(meterRegistry);

        Scope scope = new RootScopeBuilder()
                .reporter(statsReporter)
                .reportEvery(Duration.ofSeconds(15));

        scope = new TemporalMetricScoreWrapper(scope);

        var options = WorkflowServiceStubsOptions.newBuilder()
                .setTarget(endpoint)
                .setMetricsScope(scope)
                .validateAndBuildWithDefaults();
        return WorkflowServiceStubs.newServiceStubs(options);
    }

    public static TemporalWorkerFactoryBuilder createWorkerFactoryBuilder(TemporalService temporalService) {
        return TemporalWorkerFactoryBuilder.create(temporalService.getWorkflowClient());
    }

    public static TemporalWorkerBuilder createWorker(String queue) {
        return TemporalWorkerBuilder.create(queue)
                .maxWorkflowThreads(TemporalWorkerBuilder.MAX_WORKFLOW_THREADS)
                .maxActivityThreads(TemporalWorkerBuilder.MAX_ACTIVITY_THREADS);
    }

    public static TemporalWorkerBuilder createDefaultWorker() {
        return createWorker(TemporalService.DEFAULT_QUEUE);
    }

    public static TemporalWorkerBuilder createCronWorker() {
        return createWorker(TemporalService.CRON_QUEUE)
                .maxWorkflowThreads(TemporalWorkerBuilder.MAX_CRON_WORKFLOW_THREADS)
                .maxActivityThreads(TemporalWorkerBuilder.MAX_CRON_ACTIVITY_THREADS);
    }

    public static TemporalWorkerFactoryBuilder createWorkerFactoryBuilder(WorkflowClient workflowClient) {
        return TemporalWorkerFactoryBuilder.create(workflowClient);
    }
}
