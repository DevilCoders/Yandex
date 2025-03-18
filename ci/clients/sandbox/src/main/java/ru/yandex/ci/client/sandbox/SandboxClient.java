package ru.yandex.ci.client.sandbox;

import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Consumer;

import javax.annotation.Nullable;

import ru.yandex.ci.client.base.http.RetryPolicies;
import ru.yandex.ci.client.base.http.RetryPolicy;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.DelegationResultList;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.Resources;
import ru.yandex.ci.client.sandbox.api.SandboxBatchAction;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.SecretList;
import ru.yandex.ci.client.sandbox.api.TaskAuditRecord;
import ru.yandex.ci.client.sandbox.model.Semaphore;

public interface SandboxClient {

    /**
     * See https://nda.ya.ru/t/MQZeBdP53mK9Sr
     */
    int MAX_LIMIT_FOR_GET_TASKS_REQUEST = 3000;

    static RetryPolicy defaultRetryPolicy() {
        return RetryPolicies.requireAll(
                RetryPolicies.nonRetryableResponseCodes(Set.of(400, 404)),
                RetryPolicies.retryWithExponentialSleep(10, Duration.ofSeconds(1), Duration.ofSeconds(15))
        );
    }

    SandboxTaskOutput createTask(SandboxTask sandboxTask);

    BatchResult startTask(long taskId, String comment);

    List<BatchResult> startTasks(Set<Long> taskIds, String comment);

    BatchResult stopTask(long taskId, String comment);

    List<BatchResult> stopTasks(Set<Long> taskIds, String comment);

    List<BatchResult> executeBatchTaskAction(SandboxBatchAction action, Set<Long> taskIds, String comment);

    SandboxResponse<SandboxTaskOutput> getTask(long taskId);

    SandboxResponse<SandboxTaskStatus> getTaskStatus(long taskId);

    SandboxResponse<List<TaskAuditRecord>> getTaskAudit(long taskId);

    /**
     * Returns statuses of tasks.
     *
     * @param taskIds Size of taskIds is limited by {@link SandboxClient#MAX_LIMIT_FOR_GET_TASKS_REQUEST}
     */
    Map<Long, SandboxTaskOutput> getTasks(Set<Long> taskIds, Set<String> fields);

    void getTasks(Set<Long> taskIds, Set<String> fields, Consumer<Map<Long, SandboxTaskOutput>> responseConsumer);

    List<SandboxTaskOutput> getTasks(TasksFilter filter);

    Resources getTaskResources(long taskId, @Nullable String resourceType);

    long getTotalFilteredTasks(TasksFilter filter);

    List<Long> getTasksIds(TasksFilter filter);

    Semaphore getSemaphore(String id);

    void setSemaphoreCapacity(String id, long capacity, String comment);

    default void setSemaphoreCapacity(String id, int capacity, String comment) {
        setSemaphoreCapacity(id, (long) capacity, comment);
    }

    ResourceInfo getResourceInfo(long resourceId);

    DelegationResultList.DelegationResult delegateYavSecret(String secretUuid, String tvmUserTicket);

    DelegationResultList delegateYavSecrets(SecretList secretList, String tvmUserTicket);

    List<String> getCurrentUserGroups();

    String getCurrentLogin();

    int getMaxLimitForGetTasksRequest();

}
