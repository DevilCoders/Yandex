package ru.yandex.ci.tms.client;

import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.MapperFeature;
import lombok.Value;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.Headers;
import retrofit2.http.POST;
import retrofit2.http.Path;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.tms.data.SolomonAlert;

public class SolomonClient {
    private final SolomonApi api;

    private SolomonClient(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .objectMapper(
                        RetrofitClient.Builder.defaultObjectMapper()
                                .enable(MapperFeature.ACCEPT_CASE_INSENSITIVE_ENUMS)
                )
                .build(SolomonApi.class);
    }

    public static SolomonClient create(HttpClientProperties httpClientProperties) {
        return new SolomonClient(httpClientProperties);
    }

    public SolomonAlert.Status getAlertStatusCode(SolomonAlert alert) {
        return getAlertStatus(alert).getCode();
    }

    public StatusResponse getAlertStatus(SolomonAlert alert) {
        return api.getAlertState(alert.getProjectId(), alert.getAlertId()).getStatus();
    }

    public PushResponse push(RequiredMetricLabels labels, SolomonMetrics metrics) {
        return api.push(labels.getProject(), labels.getCluster(), labels.getService(), metrics);
    }

    @Value
    public static class RequiredMetricLabels {
        String project;
        String cluster;
        String service;
    }

    @Value
    private static class AlertStateResponse {
        StatusResponse status;
    }

    @Value
    public static class StatusResponse {
        SolomonAlert.Status code;
        Map<String, String> annotations;
    }

    @Value
    public static class PushResponse {
        @Nullable
        String errorMessage;
        int sensorsProcessed;
        PushStatus status;
    }

    public enum PushStatus {
        OK,
        AUTH_ERROR,
        TIMEOUT,
        NOT_200,
        CONNECT_FAILURE,
        DERIV_AND_TS,
        IPC_QUEUE_OVERFLOW,
        JSON_ERROR,
        PARSE_ERROR,
        QUOTA_ERROR,
        RESPONSE_TOO_LARGE,
        SENSOR_OVERFLOW,
        SHARD_IS_NOT_WRITABLE,
        SHARD_NOT_INITIALIZED,
        SKIP_TOO_LONG,
        SPACK_ERROR,
        UNKNOWN_ERROR,
        UNKNOWN_HOST,
        UNKNOWN_PROJECT,
        UNKNOWN_SHARD,
        UNKNOWN_STATUS_TYPE,
        UNRECOGNIZED
    }

    // https://solomon.yandex-team.ru/swagger-ui/index.html
    interface SolomonApi {

        @GET("/api/v2/projects/{projectId}/alerts/{alertId}/state/evaluation")
        AlertStateResponse getAlertState(@Path("projectId") String projectId,
                                         @Path("alertId") String alertId);

        @POST("/api/v2/push")
        @Headers("Content-Type: application/json")
        PushResponse push(
                @Query("project") String project,
                @Query("cluster") String cluster,
                @Query("service") String service,
                @Body SolomonMetrics metrics
        );
    }
}
