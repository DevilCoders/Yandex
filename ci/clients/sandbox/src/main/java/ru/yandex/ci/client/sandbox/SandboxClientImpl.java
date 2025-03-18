package ru.yandex.ci.client.sandbox;

import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import retrofit2.Response;

import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.base.http.LoggingConfig;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.sandbox.api.BatchData;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.DelegationResultList;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.Resources;
import ru.yandex.ci.client.sandbox.api.SandboxBatchAction;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.SandboxTasksListOutput;
import ru.yandex.ci.client.sandbox.api.SecretList;
import ru.yandex.ci.client.sandbox.api.SemaphoreAuditResponse;
import ru.yandex.ci.client.sandbox.api.SemaphoreResponse;
import ru.yandex.ci.client.sandbox.api.SemaphoreUpdate;
import ru.yandex.ci.client.sandbox.api.SuggestGroup;
import ru.yandex.ci.client.sandbox.api.TaskAuditRecord;
import ru.yandex.ci.client.sandbox.model.Semaphore;

public class SandboxClientImpl implements SandboxClient {
    @VisibleForTesting
    static final String API_QUOTA_CONSUMPTION_HEADER = "X-Api-Consumption-Milliseconds";

    @VisibleForTesting
    static final String API_QUOTA_HEADER = "X-Api-Quota-Milliseconds";

    private final SandboxApi api;
    private final SandboxApiV2 apiV2;
    private final SandboxClientProperties properties;

    private SandboxClientImpl(SandboxClientProperties properties) {
        var apiProperties = properties.getHttpClientProperties().withEndpoint(properties.getSandboxApiUrl());
        this.api = RetrofitClient.builder(apiProperties, getClass())
                .snakeCaseNaming()
                .withNonNumericNumbers()
                .build(SandboxApi.class);

        var apiV2Properties = properties.getHttpClientProperties().withEndpoint(properties.getSandboxApiV2Url());
        this.apiV2 = RetrofitClient.builder(apiV2Properties, getClass())
                .snakeCaseNaming()
                .withNonNumericNumbers()
                .build(SandboxApiV2.class);

        this.properties = properties;
    }

    public static SandboxClientImpl create(SandboxClientProperties properties) {
        return new SandboxClientImpl(properties);
    }

    @Override
    public SandboxTaskOutput createTask(SandboxTask sandboxTask) {
        return api.taskPost(sandboxTask);
    }

    @Override
    public BatchResult startTask(long taskId, String comment) {
        return startTasks(Set.of(taskId), comment).get(0);
    }

    @Override
    public List<BatchResult> startTasks(Set<Long> taskIds, String comment) {
        if (taskIds.isEmpty()) {
            return List.of();
        }
        return executeBatchTaskAction(SandboxBatchAction.START, taskIds, comment);
    }

    @Override
    public BatchResult stopTask(long taskId, String comment) {
        return stopTasks(Set.of(taskId), comment).get(0);
    }

    @Override
    public List<BatchResult> stopTasks(Set<Long> taskIds, String comment) {
        return executeBatchTaskAction(SandboxBatchAction.STOP, taskIds, comment);
    }

    @Override
    public List<BatchResult> executeBatchTaskAction(SandboxBatchAction action, Set<Long> taskIds, String comment) {
        try {
            var data = BatchData.builder()
                    .comment(comment)
                    .id(taskIds)
                    .build();

            var response = switch (action) {
                case START -> api.batchTasksStart(data);
                case STOP -> api.batchTasksStop(data);
            };

            Preconditions.checkState(
                    response.size() == taskIds.size(),
                    "Expected %s results, got %s",
                    taskIds.size(), response.size()
            );

            return response;
        } catch (Exception ex) {
            throw new RuntimeException(String.format("Exception while executing \"%s\" action with task ids: %s",
                    action, taskIds
            ), ex);
        }
    }

    @Override
    public SandboxResponse<SandboxTaskOutput> getTask(long taskId) {
        var response = api.taskGet(taskId, LoggingConfig.all());
        return createSandboxResponse(response);
    }

    /**
     * Using batch api, cause it is 40% faster https://st.yandex-team.ru/CI-1190#600f35014427e16b4d01fbc4
     */
    @Override
    public SandboxResponse<SandboxTaskStatus> getTaskStatus(long taskId) {
        var filter = TasksFilter.builder()
                .limit(1)
                .field("status")
                .id(taskId)
                .build();

        Response<SandboxTasksListOutput> response = api.tasksResponsePrimary(filter, LoggingConfig.all());

        SandboxTasksListOutput body = response.body();
        Preconditions.checkState(body != null);

        SandboxTaskStatus status = body.getItems().get(0).getStatus();
        return createSandboxResponse(response, status);
    }

    @Override
    public SandboxResponse<List<TaskAuditRecord>> getTaskAudit(long taskId) {
        var response = api.taskAuditResponse(taskId, LoggingConfig.allLimited());
        var body = response.body();
        Preconditions.checkState(body != null);
        return createSandboxResponse(response, body);
    }

    private <D> SandboxResponse<D> createSandboxResponse(Response<D> response) {
        return createSandboxResponse(response, response.body());
    }

    private <D> SandboxResponse<D> createSandboxResponse(Response<?> response, D data) {
        var apiQuota = getHeaderLongValue(response, API_QUOTA_HEADER, -1);
        var apiQuotaConsumption = getHeaderLongValue(response, API_QUOTA_CONSUMPTION_HEADER, -1);

        return SandboxResponse.of(data, apiQuotaConsumption, apiQuota);
    }

    private long getHeaderLongValue(Response<?> response, String header, long defaultValue) {
        return Optional.ofNullable(response.headers().get(header)).map(Long::parseLong).orElse(defaultValue);
    }

    @Override
    public Map<Long, SandboxTaskOutput> getTasks(Set<Long> taskIds, Set<String> fields) {
        Preconditions.checkArgument(
                taskIds.size() <= SandboxClient.MAX_LIMIT_FOR_GET_TASKS_REQUEST,
                "taskIds.size > %s. See https://nda.ya.ru/t/MQZeBdP53mK9Sr",
                SandboxClient.MAX_LIMIT_FOR_GET_TASKS_REQUEST
        );
        if (taskIds.isEmpty()) {
            return Map.of();
        }
        var filter = TasksFilter.builder()
                .ids(taskIds)
                .limit(taskIds.size())
                .offset(0)
                .fields(fields)
                .build();

        SandboxTasksListOutput response = api.tasksResponse(filter).body();
        Preconditions.checkState(response != null);
        return response.getItems().stream().collect(Collectors.toMap(
                SandboxTaskOutput::getId, Function.identity()
        ));
    }

    @Override
    public void getTasks(Set<Long> taskIds, Set<String> fields,
                         Consumer<Map<Long, SandboxTaskOutput>> responseConsumer) {

        Set<Long> ids = new HashSet<>();
        for (var iter = taskIds.iterator(); iter.hasNext(); ) {
            ids.add(iter.next());
            if (ids.size() != getMaxLimitForGetTasksRequest() && iter.hasNext()) {
                continue;
            }
            responseConsumer.accept(getTasks(ids, fields));
            ids.clear();
        }
    }

    @Override
    public List<SandboxTaskOutput> getTasks(TasksFilter filter) {
        var body = api.tasksResponse(filter).body();
        Preconditions.checkState(body != null);
        return body.getItems();
    }

    @Override
    public Resources getTaskResources(long taskId, String resourceType) {
        var filter = ResourceFilter.builder()
                .taskId(taskId)
                .type(resourceType)
                .build();
        var resource = apiV2.resource(filter, 1000);
        Preconditions.checkState(resource.getTotal() <= 1000,
                "found more than 1000 resources: %s in task %s", resource.getTotal(), taskId);

        return resource;
    }

    @Override
    public long getTotalFilteredTasks(TasksFilter filter) {
        var zeroFilter = filter.toBuilder().limit(0).offset(0).build();

        var body = api.tasksResponse(zeroFilter).body();
        Preconditions.checkState(body != null);
        return body.getTotal();
    }

    @Override
    public List<Long> getTasksIds(TasksFilter filter) {
        var onlyIdsFilter = filter.toBuilder().fields(Set.of("id")).build();
        var body = api.tasksResponse(onlyIdsFilter).body();
        Preconditions.checkState(body != null);

        return body.getItems().stream().map(SandboxTaskOutput::getId).collect(Collectors.toList());
    }

    @Override
    public Semaphore getSemaphore(String id) {
        SemaphoreResponse semaphoreResponse = api.semaphore(id);

        List<SemaphoreAuditResponse> semaphoreAuditResponses = api.semaphoreAudit(id);

        var last = semaphoreAuditResponses.get(semaphoreAuditResponses.size() - 1);
        return Semaphore.builder()
                .capacity(semaphoreResponse.getCapacity())
                .value(semaphoreResponse.getValue())
                .updateTime(last.getTime())
                .author(last.getAuthor())
                .build();
    }

    @Override
    public void setSemaphoreCapacity(String id, long capacity, String comment) {
        try {
            api.semaphorePut(id, SemaphoreUpdate.builder().capacity(capacity).event(comment).build());
        } catch (HttpException ex) {
            // TODO Remove in CI-615
            if (ex.getCause() instanceof IOException && ex.getCause().getMessage() != null
                    && ex.getCause().getMessage().equals("unexpected content length header with 204 response")) {
                return;
            }

            throw ex;
        }
    }

    @Override
    public ResourceInfo getResourceInfo(long resourceId) {
        return api.resource(resourceId);
    }

    @Override
    public DelegationResultList.DelegationResult delegateYavSecret(String secretUuid, String tvmUserTicket) {
        SecretList secrets = SecretList.withSecrets(secretUuid);
        DelegationResultList result = delegateYavSecrets(secrets, tvmUserTicket);
        Preconditions.checkState(result.getItems().size() == 1, "response should have single item, got %s", result);

        return result.getItems().get(0);
    }

    @Override
    public DelegationResultList delegateYavSecrets(SecretList secretList, String tvmUserTicket) {
        return api.delegateSecrets(secretList, tvmUserTicket);
    }

    @Override
    public List<String> getCurrentUserGroups() {
        return api.userCurrentGroups().stream().map(SuggestGroup::getName).collect(Collectors.toList());
    }

    @Override
    public String getCurrentLogin() {
        return api.userCurrent().getLogin();
    }

    @Override
    public int getMaxLimitForGetTasksRequest() {
        return properties.getMaxLimitForGetTasksRequest();
    }

}

