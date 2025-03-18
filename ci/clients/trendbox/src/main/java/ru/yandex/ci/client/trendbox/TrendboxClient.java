package ru.yandex.ci.client.trendbox;

import java.util.ArrayList;
import java.util.List;

import retrofit2.http.GET;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.trendbox.model.TrendboxWorkflow;

public class TrendboxClient {

    private final TrendboxApi api;

    private TrendboxClient(HttpClientProperties clientProperties) {
        api = RetrofitClient.builder(clientProperties, getClass())
                .build(TrendboxApi.class);
    }

    public static TrendboxClient create(HttpClientProperties httpClientProperties) {
        return new TrendboxClient(httpClientProperties);
    }

    public List<TrendboxWorkflow> getWorkflows() {
        List<TrendboxWorkflow> workflows = new ArrayList<>();
        //Trendbox возвращает не больше 50 за раз
        for (int page = 1; ; page++) {
            TrendboxListResponse<TrendboxWorkflow> response = api.getWorkflows(page);
            if (response.getItems().isEmpty()) {
                break;
            }
            workflows.addAll(response.getItems());
        }
        return workflows;
    }

    interface TrendboxApi {
        @GET("api/v1/workflows")
        TrendboxListResponse<TrendboxWorkflow> getWorkflows(@Query("page") int page);
    }

}
