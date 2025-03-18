package ru.yandex.ci.client.observer;

import java.util.List;

import javax.annotation.Nonnull;

import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.POST;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class ObserverClientImpl implements ObserverClient {

    @Nonnull
    private final ObserverApi api;

    private ObserverClientImpl(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(ObserverApi.class);
    }

    public static ObserverClientImpl create(HttpClientProperties httpClientProperties) {
        return new ObserverClientImpl(httpClientProperties);
    }

    @Override
    public List<CheckRevisionsDto> getNotUsedRevisions(
            String startRevision,
            String duration,
            int revisionsPerHour,
            String namespace
    ) {
        return api.getNotUsedRevisions(startRevision, duration, revisionsPerHour, namespace);
    }

    @Override
    public void markAsUsed(String rightRevision, String leftRevision, String namespace, String flowLaunchId) {
        api.markAsUsed(rightRevision, leftRevision, namespace, flowLaunchId);
    }

    @Override
    public List<UsedRevisionResponseDto> findUsedRevisions(List<String> rightRevisions, String namespace) {
        return api.findUsedRevisions(new GetUsedRevisionsRequestDto(rightRevisions, namespace));
    }

    @SuppressWarnings("UnusedReturnValue")
    interface ObserverApi {

        @GET("/api/v1/stress-test/not-used-revisions")
        List<CheckRevisionsDto> getNotUsedRevisions(
                @Query("startRevision") String startRevision,
                @Query("duration") String duration,
                @Query("revisionPerHour") int revisionsPerHour,
                @Query("namespace") String namespace
        );

        @POST("/api/v1/stress-test/mark-as-used")
        Response<Void> markAsUsed(
                @Query("rightRevision") String rightRevision,
                @Query("leftRevision") String leftRevision,
                @Query("namespace") String namespace,
                @Query("flowLaunchId") String flowLaunchId
        );

        @POST("/api/v1/stress-test/find-used-revisions")
        List<UsedRevisionResponseDto> findUsedRevisions(@Body GetUsedRevisionsRequestDto request);

    }
}
