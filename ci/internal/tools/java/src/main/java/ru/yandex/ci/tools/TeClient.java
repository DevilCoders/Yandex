package ru.yandex.ci.tools;

import java.io.IOException;
import java.io.UncheckedIOException;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import okhttp3.ResponseBody;
import retrofit2.Response;
import retrofit2.http.GET;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.Idempotent;
import ru.yandex.ci.client.base.http.RetrofitClient;

public class TeClient {

    private final Api api;

    public TeClient(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(Api.class);
    }

    public String restartCheck(String id, String cacheNamespace) {
        var response = api.restartCheck(id, Strings.isNullOrEmpty(cacheNamespace) ? null : cacheNamespace);
        ResponseBody body = response.body();
        Preconditions.checkState(body != null);
        try {
            return body.string().trim();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    interface Api {
        @Idempotent(false)
        @GET("/api/restart_check")
        Response<ResponseBody> restartCheck(
                @Query("check_id") String checkId,
                @Query("cache_namespace") @Nullable String cacheNamespace);

    }

}
