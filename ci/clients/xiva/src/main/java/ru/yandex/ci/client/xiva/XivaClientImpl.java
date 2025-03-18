package ru.yandex.ci.client.xiva;

import java.time.Duration;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import retrofit2.Response;
import retrofit2.http.Body;
import retrofit2.http.POST;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class XivaClientImpl implements XivaClient {

    @Nonnull
    private final String service;
    @Nonnull
    private final XivaApi api;

    private XivaClientImpl(String service, HttpClientProperties httpClientProperties) {
        this.service = service;
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(XivaApi.class);
    }

    public static XivaClientImpl create(String service, HttpClientProperties httpClientProperties) {
        return new XivaClientImpl(service, httpClientProperties);
    }

    @Override
    public void send(@Nonnull String topic, @Nonnull SendRequest request, @Nullable String shortTitle) {
        Preconditions.checkArgument(!topic.isEmpty(), "Topic is empty");
        api.send(service, topic, request, shortTitle, null);
    }

    @Override
    public String getService() {
        return service;
    }

    interface XivaApi {
        /**
         * See https://console.push.yandex-team.ru/#api-reference-send-topic
         */
        @POST("/v2/send")
        Response<Void> send(
                @Query("service") @Nonnull String service,
                @Query("topic") @Nonnull String topic,
                @Body @Nonnull SendRequest request,
                @Query("event") @Nullable String event,
                @Query("ttl") @Nullable Duration ttl
        );
    }
}
