package ru.yandex.ci.tms.task.potato;

import java.util.Map;

import javax.annotation.Nullable;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import retrofit2.http.GET;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.tms.task.potato.client.Status;

public class PotatoClientImpl implements PotatoClient {
    private static final Logger log = LoggerFactory.getLogger(PotatoClientImpl.class);

    private final PotatoApi api;

    private PotatoClientImpl(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(PotatoApi.class);
    }

    public static PotatoClient create(HttpClientProperties httpClientProperties) {
        return new PotatoClientImpl(httpClientProperties);
    }

    @Override
    public Map<String, Status> healthCheck(@Nullable String namespace) {
        log.info("Query health checks with namespace {}", namespace);

        var checks = api.healthCheck(namespace);
        log.info("Found {} health checks", checks.size());
        return checks;
    }

    interface PotatoApi {
        @GET("/telemetry/healthcheck")
        Map<String, Status> healthCheck(@Query("namespace") String namespace);
    }
}
