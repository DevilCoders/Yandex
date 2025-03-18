package ru.yandex.ci.client.sandbox;

import retrofit2.http.GET;
import retrofit2.http.Headers;
import retrofit2.http.Query;
import retrofit2.http.QueryMap;

import ru.yandex.ci.client.sandbox.api.Resources;

public interface SandboxApiV2 {
    // `limit` is required
    @GET("resource")
    @Headers("X-Read-Preference: PRIMARY")
    Resources resource(@QueryMap ResourceFilter filter, @Query("limit") int limit);
}
