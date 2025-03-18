package ru.yandex.ci.client.base.http;

import java.time.Duration;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;
import lombok.With;

@SuppressWarnings("ReferenceEquality")
@Value
@lombok.Builder(toBuilder = true)
public class HttpClientProperties {

    @With
    @Nonnull
    String endpoint;

    @With
    @Nullable
    AuthProvider authProvider;

    @With
    @Nonnull
    RetryPolicy retryPolicy;

    @Nonnull
    Duration requestTimeout;

    @Nonnull
    Duration connectionTimeout;

    @Nullable
    Boolean followRedirects;

    @Nullable
    CallsMonitorSource callsMonitorSource;

    @Nullable
    String clientNameSuffix;

    public static HttpClientProperties ofEndpoint(String endpoint) {
        return HttpClientProperties.builder()
                .endpoint(endpoint)
                .build();
    }

    public static HttpClientProperties ofEndpoint(String endpoint, CallsMonitorSource callsMonitorSource) {
        return HttpClientProperties.builder()
                .endpoint(endpoint)
                .callsMonitorSource(callsMonitorSource)
                .build();
    }

    public static class Builder {
        {
            retryPolicy = RetryPolicies.defaultPolicy();
            connectionTimeout = Duration.ofSeconds(10);
            requestTimeout = Duration.ofMinutes(1);
        }

    }

}
