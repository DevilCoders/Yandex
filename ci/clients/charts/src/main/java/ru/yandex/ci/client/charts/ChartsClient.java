package ru.yandex.ci.client.charts;

import java.util.List;

import com.google.common.base.Preconditions;
import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.POST;
import retrofit2.http.Path;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.charts.model.ChartsCommentRequest;
import ru.yandex.ci.client.charts.model.ChartsCreateCommentResponse;
import ru.yandex.ci.client.charts.model.ChartsGetCommentRequest;
import ru.yandex.ci.client.charts.model.ChartsGetCommentResponse;

import static ru.yandex.ci.client.charts.model.jackson.ChartsInstantSerializer.DATE_TIME_FORMATTER;

public class ChartsClient {

    private final ChartsApi api;

    private ChartsClient(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(ChartsApi.class);
    }

    public static ChartsClient create(HttpClientProperties httpClientProperties) {
        return new ChartsClient(httpClientProperties);
    }

    public ChartsCreateCommentResponse createComment(ChartsCommentRequest request) {
        Preconditions.checkNotNull(request.getFeed(), new RuntimeException("feed could not be empty"));
        Preconditions.checkNotNull(request.getType(), new RuntimeException("type could not be empty"));

        return api.createComment(request);
    }

    public List<ChartsGetCommentResponse> getComments(ChartsGetCommentRequest request) {
        return api.getComments(request.getFeed(),
                DATE_TIME_FORMATTER.format(request.getDateFrom()),
                DATE_TIME_FORMATTER.format(request.getDateTo()));
    }

    public void updateComment(String commentId, ChartsCommentRequest request) {
        Preconditions.checkNotNull(commentId, new RuntimeException("id could not be empty"));
        Preconditions.checkNotNull(request.getFeed(), new RuntimeException("feed could not be empty"));

        api.updateComment(commentId, request);
    }


    interface ChartsApi {
        @POST("/api/v1/comments")
        ChartsCreateCommentResponse createComment(@Body ChartsCommentRequest request);


        @GET("/api/v1/comments")
        List<ChartsGetCommentResponse> getComments(@Query("feed") String feed,
                                                   @Query("dateFrom") String dateFrom,
                                                   @Query("dateTo") String dateTo);

        @POST("/api/v1/comments/{commentId}")
        Response<Void> updateComment(@Path("commentId") String commentId,
                                     @Body ChartsCommentRequest request);
    }
}

