package ru.yandex.ci.client.sandbox;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.client.base.http.HttpClientProperties;

@Value
@lombok.Builder
public class SandboxClientProperties {

    @Nonnull
    String sandboxApiUrl;

    @Nonnull
    String sandboxApiV2Url;

    @Nonnull
    HttpClientProperties httpClientProperties;

    int maxLimitForGetTasksRequest;

    public static class Builder {
        {
            maxLimitForGetTasksRequest = SandboxClient.MAX_LIMIT_FOR_GET_TASKS_REQUEST;
        }
    }
}
