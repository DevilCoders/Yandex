package ru.yandex.ci.common.temporal;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;
import java.time.Duration;
import java.time.Instant;
import java.util.function.Function;

import com.cronutils.model.Cron;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.CharMatcher;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.protobuf.TextFormat;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import io.temporal.activity.ActivityOptions;
import io.temporal.api.common.v1.Payload;
import io.temporal.api.common.v1.WorkflowExecution;
import io.temporal.api.workflow.v1.WorkflowExecutionInfo;
import io.temporal.api.workflowservice.v1.DescribeWorkflowExecutionRequest;
import io.temporal.api.workflowservice.v1.ListArchivedWorkflowExecutionsRequest;
import io.temporal.api.workflowservice.v1.WorkflowServiceGrpc;
import io.temporal.client.WorkflowClient;
import io.temporal.client.WorkflowExecutionAlreadyStarted;
import io.temporal.client.WorkflowOptions;
import io.temporal.common.RetryOptions;
import io.temporal.common.converter.DataConverter;
import io.temporal.workflow.Functions;
import io.temporal.workflow.Workflow;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.Tx;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.common.temporal.ydb.TemporalDb;
import ru.yandex.ci.common.temporal.ydb.TemporalLaunchQueueEntity;

@Slf4j
public class TemporalService {

    public static final String DEFAULT_QUEUE = "default";
    public static final String CRON_QUEUE = "cron";

    private static final RetryOptions RETRY_OPTIONS = RetryOptions.newBuilder()
            .setInitialInterval(Duration.ofSeconds(2))
            .setMaximumInterval(Duration.ofHours(1))
            .setMaximumAttempts(Integer.MAX_VALUE)
            /*
                10 retries: backoff ~= 12 sec
                20 retries: backoff ~= 76 sec
                25 retries: backoff ~= 3 min
                30 retries: backoff ~= 8 min
                40 retries: backoff ~= 40 min
             */
            .setBackoffCoefficient(1.2)
            .build();

    private static final Duration DEFAULT_HEARTBEAT_TIMEOUT = Duration.ofMinutes(2);

    private final WorkflowClient workflowClient;
    private final TemporalDb temporalDb;
    private final DataConverter dataConverter;
    private final String uiUrlBase;

    public TemporalService(WorkflowClient workflowClient, TemporalDb temporalDb, DataConverter dataConverter,
                           String uiUrlBase) {
        this.workflowClient = workflowClient;
        this.temporalDb = temporalDb;
        this.dataConverter = dataConverter;
        this.uiUrlBase = CharMatcher.is('/').trimTrailingFrom(uiUrlBase);
    }


    public <T extends BaseTemporalWorkflow<I>, I extends BaseTemporalWorkflow.Id> void startInTx(
            Class<T> type,
            Function<T, Functions.Proc1<I>> runFunction,
            I input
    ) {
        startInTx(type, runFunction, input, DEFAULT_QUEUE);
    }

    /**
     * Start task only in if transaction commits successfully .
     */
    public <T extends BaseTemporalWorkflow<I>, I extends BaseTemporalWorkflow.Id> void startInTx(
            Class<T> type,
            Function<T, Functions.Proc1<I>> runFunction,
            I input,
            String taskQueue
    ) {

        var entity = toLaunchQueueEntity(type, runFunction, input, taskQueue);

        Tx tx = Tx.Current.get();

        log.info(
                "Scheduling workflow {}:{} start after tx commit. Launch queue entry Id {}",
                type.getSimpleName(), input.getTemporalWorkflowId(), entity.getId()
        );
        temporalDb.temporalLaunchQueue().save(entity);

        tx.defer(() -> {
            log.info(
                    "Transaction committed. Starting workflow {}:{}. Launch queue entry Id {}",
                    type.getSimpleName(), input.getTemporalWorkflowId(), entity.getId()
            );
            startLaunchQueueEntity(entity);
        });
    }

    @SuppressWarnings("unchecked")
    public <T extends BaseTemporalWorkflow<I>,
            I extends BaseTemporalWorkflow.Id> void startLaunchQueueEntity(TemporalLaunchQueueEntity entity) {
        Class<T> type = null;
        Class<I> payloadType = null;

        try {
            type = (Class<T>) Class.forName(entity.getType());
            payloadType = (Class<I>) Class.forName(entity.getPayloadType());
        } catch (ClassNotFoundException e) {
            throw new RuntimeException(e);
        }
        I input = fromTextPayload(entity.getPayload(), payloadType);

        startDeduplicated(type, toRunFunction(entity.getRunMethod()), input, entity.getQueue());
        log.info("Task started. Removing launch queue entity {} from DB", entity.getId());
        temporalDb.tx(() -> temporalDb.temporalLaunchQueue().delete(entity.getId()));

    }


    public static <T extends BaseTemporalWorkflow<I>,
            I extends BaseTemporalWorkflow.Id> Function<T, Functions.Proc1<I>> toRunFunction(String runMethod) {
        return workflow -> input -> {
            try {
                workflow.getClass().getMethod(runMethod, input.getClass()).invoke(workflow, input);
            } catch (IllegalAccessException | InvocationTargetException | NoSuchMethodException e) {
                throw new RuntimeException(e);
            }
        };
    }

    /**
     * Starts task in temporal. Deduplicate task using input.getTemporalWorkflowId().
     * This method can be used with at-least semantic.
     * Don't use it if you need to run task only if transaction successfully commits.
     */
    @CanIgnoreReturnValue
    public <T extends BaseTemporalWorkflow<I>, I extends BaseTemporalWorkflow.Id> WorkflowExecution startDeduplicated(
            Class<T> type,
            Function<T, Functions.Proc1<I>> runFunction,
            I input
    ) {
        return startDeduplicated(type, runFunction, input, DEFAULT_QUEUE);
    }


    /**
     * Starts task in temporal. Deduplicate task using input.getTemporalWorkflowId().
     * This method can be used with at-least semantic.
     * Don't use it if you need to run task only if transaction successfully commits.
     */
    @CanIgnoreReturnValue
    public <T extends BaseTemporalWorkflow<I>, I extends BaseTemporalWorkflow.Id> WorkflowExecution startDeduplicated(
            Class<T> type,
            Function<T, Functions.Proc1<I>> runFunction,
            I input,
            String taskQueue
    ) {

        WorkflowOptions options = WorkflowOptions.newBuilder()
                .setTaskQueue(taskQueue)
                .setWorkflowId(toId(type, input))
                .setRetryOptions(RETRY_OPTIONS)
                .validateBuildWithDefaults();

        return startDeduplicatedInternal(
                type,
                options,
                t -> WorkflowClient.start(runFunction.apply(t), input)
        );
    }

    @CanIgnoreReturnValue
    public <T extends BaseTemporalCronWorkflow> WorkflowExecution registerCronTask(
            Class<T> type,
            Function<T, Functions.Proc> runFunction,
            Cron cron,
            Duration timeout
    ) {
        WorkflowOptions options = WorkflowOptions.newBuilder()
                .setTaskQueue(CRON_QUEUE)
                .setWorkflowId(type.getSimpleName() + "-cron")
                .setRetryOptions(RETRY_OPTIONS)
                .setCronSchedule(cron.asString())
                .setWorkflowRunTimeout(timeout)
                .validateBuildWithDefaults();

        return startDeduplicatedInternal(
                type,
                options,
                t -> WorkflowClient.start(runFunction.apply(t))
        );

    }

    private <T> WorkflowExecution startDeduplicatedInternal(
            Class<T> type,
            WorkflowOptions options,
            Function<T, WorkflowExecution> startFunction
    ) {
        T workflowStub = workflowClient.newWorkflowStub(type, options);

        WorkflowExecution execution;
        try {
            execution = startFunction.apply(workflowStub);
        } catch (WorkflowExecutionAlreadyStarted e) {
            execution = e.getExecution();
            log.warn(
                    "Workflow {} already exists. Launch deduplicated. WorkflowID: {} RunID: {}",
                    type.getSimpleName(), execution.getWorkflowId(), execution.getRunId()
            );
            return execution;
        }

        log.info(
                "Workflow {} started. WorkflowID: {} RunID: {}",
                type.getSimpleName(), execution.getWorkflowId(), execution.getRunId()
        );
        return execution;
    }

    @SuppressWarnings("unchecked")
    public <T extends BaseTemporalWorkflow<I>,
            I extends BaseTemporalWorkflow.Id> TemporalLaunchQueueEntity toLaunchQueueEntity(
            Class<T> type,
            Function<T, Functions.Proc1<I>> runFunction,
            I input,
            String taskQueue
    ) {

        var invocationHandler = new TemporalParamsSerializerInvocationHandler();
        T proxy = (T) Proxy.newProxyInstance(
                type.getClassLoader(),
                new Class<?>[]{type},
                invocationHandler
        );
        runFunction.apply(proxy).apply(input);

        return TemporalLaunchQueueEntity.builder()
                .id(TemporalLaunchQueueEntity.Id.of(type, input, toId(type, input)))
                .type(type.getCanonicalName())
                .runMethod(invocationHandler.getMethod().getName())
                .payloadType(input.getClass().getCanonicalName())
                .payload(toTextPayload(input))
                .enqueueTimeSeconds(Instant.now().getEpochSecond())
                .queue(taskQueue)
                .build();
    }

    private static <T extends BaseTemporalWorkflow<I>, I extends BaseTemporalWorkflow.Id> String toId(
            Class<T> type,
            I input
    ) {
        return type.getSimpleName() + "-" + input.getTemporalWorkflowId();
    }

    @VisibleForTesting
    private <I extends BaseTemporalWorkflow.Id> String toTextPayload(I input) {
        var payload = dataConverter.toPayload(input).orElseThrow(
                () -> new RuntimeException("Failed to serialize input: " + input)
        );
        return TextFormat.printer().printToString(payload);
    }

    @VisibleForTesting
    <I extends BaseTemporalWorkflow.Id> I fromTextPayload(String text, Class<I> payloadType) {
        try {
            var payload = TextFormat.parse(text, Payload.class);
            return dataConverter.fromPayload(payload, payloadType, payloadType);
        } catch (TextFormat.ParseException e) {
            throw new RuntimeException(e);
        }
    }

    public static <T> T createActivity(Class<T> activityInterface, Duration timeout) {
        return createActivity(activityInterface, timeout, DEFAULT_HEARTBEAT_TIMEOUT);
    }

    public static <T> T createActivity(Class<T> activityInterface, Duration timeout, Duration heartbeatTimeout) {
        return Workflow.newActivityStub(
                activityInterface,
                ActivityOptions.newBuilder()
                        .setStartToCloseTimeout(timeout)
                        .setHeartbeatTimeout(heartbeatTimeout)
                        .setRetryOptions(RETRY_OPTIONS)
                        .build()
        );
    }

    private WorkflowServiceGrpc.WorkflowServiceBlockingStub workflowStub() {
        return workflowClient.getWorkflowServiceStubs().blockingStub();
    }

    public WorkflowExecutionInfo getWorkflowExecutionInfo(String workflowId) {
        try {
            return getActualWorkflowExecutionInfo(workflowId);
        } catch (StatusRuntimeException e) {
            if (e.getStatus().getCode() != Status.Code.NOT_FOUND) {
                throw e;
            }
            return getArchivedWorkflowExecutionInfo(workflowId);
        }
    }

    private WorkflowExecutionInfo getActualWorkflowExecutionInfo(String workflowId) {
        var response = workflowStub().describeWorkflowExecution(
                DescribeWorkflowExecutionRequest.newBuilder()
                        .setNamespace(workflowClient.getOptions().getNamespace())
                        .setExecution(WorkflowExecution.newBuilder().setWorkflowId(workflowId).build())
                        .build()
        );

        return response.getWorkflowExecutionInfo();
    }

    private WorkflowExecutionInfo getArchivedWorkflowExecutionInfo(String workflowId) {
        String query = "WorkflowId = '" + workflowId + "'";

        var response = workflowStub().listArchivedWorkflowExecutions(
                ListArchivedWorkflowExecutionsRequest.newBuilder()
                        .setNamespace(workflowClient.getOptions().getNamespace())
                        .setQuery(query)
                        .setPageSize(10)
                        .build()
        );
        var executions = response.getExecutionsList();

        if (executions.isEmpty()) {
            throw GrpcUtils.notFoundException("No workflow found with id: " + workflowId);
        }
        if (executions.size() > 1) {
            throw GrpcUtils.failedPreconditionException("More than one execution found for id: " + workflowId);
        }
        return executions.get(0);
    }

    public WorkflowClient getWorkflowClient() {
        return workflowClient;
    }

    public String getWorkflowUrl(String workflowId) {
        return uiUrlBase + "/namespaces/" + workflowClient.getOptions().getNamespace() +
                "/workflows?range=last-3-months&status=ALL&workflowId=" + workflowId;
    }
}
