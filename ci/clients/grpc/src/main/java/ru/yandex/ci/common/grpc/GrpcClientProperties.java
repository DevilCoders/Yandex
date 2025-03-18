package ru.yandex.ci.common.grpc;

import java.time.Duration;
import java.util.List;
import java.util.concurrent.Executor;
import java.util.function.Consumer;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import io.grpc.CallCredentials;
import io.grpc.ClientInterceptor;
import io.grpc.ManagedChannel;
import io.grpc.internal.AbstractManagedChannelImplBuilder;
import lombok.Singular;
import lombok.Value;
import org.springframework.core.io.Resource;

@Value
@lombok.Builder(toBuilder = true)
public class GrpcClientProperties {
    static final String INPROCESS_PREFIX = "inprocess-";

    @Nonnull
    String endpoint;

    @Nullable
    String userAgent;

    @Nonnull
    Duration connectTimeout;

    @Nonnull
    Duration keepAliveTime;

    @Nonnull
    Duration keepAliveTimeout;

    @Nonnull
    Duration awaitTermination; // Termination timeout

    @Nonnull
    Duration deadlineAfter; // Maximum execution time of each request

    // Additional service configuration.
    // Retries is evaluated as `min(grpcServiceConfig.retryPolicy.maxAttempts, this.maxRetryAttempts)`.
    // See https://nda.ya.ru/t/0lcqYDiX4EDYqT
    @Nullable
    Resource grpcServiceConfig;

    int maxRetryAttempts;
    int maxInboundMessageSizeInBytes;
    boolean plainText;

    @Nullable
    CallCredentials callCredentials;

    @Nullable
    Executor executor;

    @Singular
    List<Consumer<ManagedChannel>> listeners;

    @Singular
    List<ClientInterceptor> interceptors;

    @Singular
    List<Consumer<AbstractManagedChannelImplBuilder<?>>> configurers;

    public static GrpcClientProperties ofEndpoint(String endpoint) {
        return GrpcClientProperties.builder()
                .endpoint(endpoint)
                .build();
    }

    public static class Builder {
        {
            connectTimeout = Duration.ofSeconds(1);
            keepAliveTime = Duration.ofMinutes(10);
            keepAliveTimeout = Duration.ofSeconds(1);
            awaitTermination = Duration.ofSeconds(30);
            deadlineAfter = Duration.ofSeconds(30);
            maxInboundMessageSizeInBytes = 10 * 1024 * 1024; // 10 MiB
            maxRetryAttempts = 3;
            plainText = true;
        }
    }

}
