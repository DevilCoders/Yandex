package ru.yandex.ci.client.sandbox;

import java.io.InputStream;
import java.util.Objects;

import okhttp3.ResponseBody;
import retrofit2.Response;
import retrofit2.http.GET;
import retrofit2.http.Path;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class ProxySandboxClientImpl implements ProxySandboxClient {

    private final SandboxProxyApi api;

    private ProxySandboxClientImpl(HttpClientProperties httpClientProperties) {
        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .build(SandboxProxyApi.class);
    }

    public static ProxySandboxClient create(HttpClientProperties httpClientProperties) {
        return new ProxySandboxClientImpl(httpClientProperties);
    }

    @Override
    public CloseableResource downloadResource(long resourceId) {
        var body = api.downloadResource(resourceId).body();
        var stream = Objects.requireNonNull(body, "Response body cannot be null").byteStream();
        return new CloseableResource() {
            @Override
            public InputStream getStream() {
                return stream;
            }

            @Override
            public void close() {
                body.close();
            }
        };
    }

    interface SandboxProxyApi {
        @GET("/{resourceId}")
        Response<ResponseBody> downloadResource(@Path("resourceId") long resourceId);
    }

}
