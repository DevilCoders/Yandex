package ru.yandex.ci.client.sandbox;

import java.util.List;

import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.Header;
import retrofit2.http.Headers;
import retrofit2.http.POST;
import retrofit2.http.PUT;
import retrofit2.http.Path;
import retrofit2.http.Query;
import retrofit2.http.QueryMap;
import retrofit2.http.Tag;

import ru.yandex.ci.client.base.http.LoggingConfig;
import ru.yandex.ci.client.sandbox.api.BatchData;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.CurrentUserResponse;
import ru.yandex.ci.client.sandbox.api.DelegationResultList;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.Resources;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTasksListOutput;
import ru.yandex.ci.client.sandbox.api.SecretList;
import ru.yandex.ci.client.sandbox.api.SemaphoreAuditResponse;
import ru.yandex.ci.client.sandbox.api.SemaphoreResponse;
import ru.yandex.ci.client.sandbox.api.SemaphoreUpdate;
import ru.yandex.ci.client.sandbox.api.SuggestGroup;
import ru.yandex.ci.client.sandbox.api.TaskAuditRecord;
import ru.yandex.ci.client.tvm.TvmHeaders;

public interface SandboxApi {
    @GET("semaphore/{id}")
    SemaphoreResponse semaphore(@Path("id") String id);

    @PUT("semaphore/{id}")
    @Headers("X-Request-Updated-Data: true")
    SemaphoreResponse semaphorePut(@Path("id") String id, @Body SemaphoreUpdate update);

    @GET("resource/{id}")
    ResourceInfo resource(@Path("id") long resourceId);

    @GET("semaphore/{id}/audit")
    List<SemaphoreAuditResponse> semaphoreAudit(@Path("id") String id);

    @GET("task/{taskId}/resources")
    Resources taskResources(@Path("taskId") long taskId);

    @GET("task/{taskId}")
    @Headers("X-Read-Preference: PRIMARY")
    Response<SandboxTaskOutput> taskGet(
            @Path("taskId") long taskId,
            @Tag LoggingConfig loggingConfig
    );

    @POST("task")
    SandboxTaskOutput taskPost(@Body SandboxTask task);

    @GET("task")
    Response<SandboxTasksListOutput> tasks(@Query("limit") int limit,
                                           @Query("fields") String fields,
                                           @Query("id") String id);

    @GET("task")
    Response<SandboxTasksListOutput> tasksResponse(@QueryMap TasksFilter filter);

    @GET("task")
    @Headers("X-Read-Preference: PRIMARY")
    Response<SandboxTasksListOutput> tasksResponsePrimary(
            @QueryMap TasksFilter filter,
            @Tag LoggingConfig loggingConfig
    );

    @GET("task/audit")
    Response<List<TaskAuditRecord>> taskAuditResponse(
            @Query("id") long taskId,
            @Tag LoggingConfig loggingConfig
    );

    @GET("user/current")
    CurrentUserResponse userCurrent();

    @GET("user/current/groups")
    List<SuggestGroup> userCurrentGroups();

    @POST("yav/tokens")
    DelegationResultList delegateSecrets(@Body SecretList secrets, @Header(TvmHeaders.USER_TICKET) String userTicket);

    @PUT("batch/tasks/stop")
    List<BatchResult> batchTasksStop(@Body BatchData batchData);

    @PUT("batch/tasks/start")
    List<BatchResult> batchTasksStart(@Body BatchData batchData);
}
