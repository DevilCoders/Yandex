package ru.yandex.ci.client.rm;

import java.util.List;

import lombok.Value;
import retrofit2.http.GET;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class RmClient {

    private final RmApi rmApi;

    private RmClient(HttpClientProperties httpClientProperties) {
        this.rmApi = RetrofitClient.builder(httpClientProperties, getClass())
                .build(RmApi.class);
    }

    public static RmClient create(HttpClientProperties httpClientProperties) {
        return new RmClient(httpClientProperties);
    }

    public List<RmComponent> getComponents() {
        return rmApi.getComponents().getComponents();
    }

    @Value
    private static class GetComponentsResponse {
        List<RmComponent> components;
    }

    interface RmApi {
        @GET("/api/release_engine.services.Model/getComponents")
        GetComponentsResponse getComponents();
    }
}
