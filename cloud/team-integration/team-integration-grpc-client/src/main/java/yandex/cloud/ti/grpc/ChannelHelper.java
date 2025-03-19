package yandex.cloud.ti.grpc;

import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

import io.grpc.ClientInterceptor;
import io.grpc.ManagedChannelBuilder;
import io.grpc.Status;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.netty.NettyChannelBuilder;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.HeaderClientInterceptor;
import yandex.cloud.grpc.LoggingInterceptor;
import yandex.cloud.grpc.MetricsInterceptor;
import yandex.cloud.grpc.client.ClientConfig;
import yandex.cloud.grpc.client.retry.MethodConfig;
import yandex.cloud.grpc.client.retry.MethodName;
import yandex.cloud.grpc.client.retry.RetryPolicy;
import yandex.cloud.grpc.client.retry.ServiceConfig;
import yandex.cloud.log.RidInterceptor;
import yandex.cloud.tracing.TracingInterceptor;

public class ChannelHelper {
    // todo client-specific interceptors
    //  idempotency key
    //      as HeaderClientInterceptor.idempotencyKey
    //  cloud auth
    //      as HeaderClientInterceptor.authorization(() -> token == null ? null : "Bearer " + token.getToken())
    //      or proper CallCredentials on stub
    //  tvm auth
    //      TvmClientInterceptor?
    //      or proper CallCredentials on stub

    public static @NotNull ManagedChannelBuilder<?> configure(
            @NotNull ManagedChannelBuilder<?> builder,
            @NotNull String applicationName,
            @NotNull String clientName,
            ClientInterceptor... clientInterceptors
    ) {
//        builder = configureTransportSecurity(builder,
//                tls
//        );
        builder = configureUserAgent(builder,
                applicationName
        );
        builder = configureDefaultKeepAlive(builder);
//        builder = configureDefaultRetries(builder);
        builder = builder.intercept(new DefaultDeadlineInterceptor(
                // todo sync with default retries
                Duration.ofSeconds(5).multipliedBy(5)
        ));
        builder = configureInterceptors(builder,
                clientInterceptors
        );
        builder = configureDefaultInterceptors(builder,
                applicationName,
                clientName
        );
        return builder;
    }

    public static @NotNull ManagedChannelBuilder<?> createNettyChannelBuilder(
            @NotNull ClientConfig clientConfig
    ) {
        ManagedChannelBuilder<?> builder = NettyChannelBuilder.forAddress(clientConfig.getHost(), clientConfig.getPort());
        return configureTransportSecurity(builder, clientConfig.isTls());
    }

    public static @NotNull ManagedChannelBuilder<?> createInProcessChannelBuilder(
            @NotNull String name
    ) {
        ManagedChannelBuilder<?> builder = InProcessChannelBuilder.forName(name);
        return configureTransportSecurity(builder, false);
    }

    public static @NotNull ManagedChannelBuilder<?> configureTransportSecurity(
            @NotNull ManagedChannelBuilder<?> builder,
            boolean tls
    ) {
        if (tls) {
            return builder.useTransportSecurity();
        } else {
            return builder.usePlaintext();
        }
    }

    public static @NotNull ManagedChannelBuilder<?> configureUserAgent(
            @NotNull ManagedChannelBuilder<?> builder,
            @NotNull String applicationName
    ) {
        return builder.userAgent(applicationName);
    }

    public static @NotNull ManagedChannelBuilder<?> configureKeepAlive(
            @NotNull ManagedChannelBuilder<?> builder,
            @NotNull Duration keepAliveTime,
            @NotNull Duration keepAliveTimeout
    ) {
        if (keepAliveTime.compareTo(Duration.ZERO) > 0) {
            builder = builder
                    .keepAliveTime(keepAliveTime.toNanos(), TimeUnit.NANOSECONDS)
                    .keepAliveTimeout(keepAliveTimeout.toNanos(), TimeUnit.NANOSECONDS)
                    .keepAliveWithoutCalls(true);
        }
        return builder;
    }

    public static @NotNull ManagedChannelBuilder<?> configureDefaultKeepAlive(
            @NotNull ManagedChannelBuilder<?> builder
    ) {
        return configureKeepAlive(builder,
                // todo extract keepAlive into constants
                Duration.ofSeconds(10),
                Duration.ofSeconds(1)
        );
    }

    public static @NotNull ManagedChannelBuilder<?> configureRetries(
            @NotNull ManagedChannelBuilder<?> builder,
            int maxRetryAttempts,
            @NotNull Map<String, ?> serviceConfigMap
    ) {
        return builder.enableRetry()
                .maxRetryAttempts(maxRetryAttempts)
//                .maxHedgedAttempts(maxRetryAttempts)
                .defaultServiceConfig(serviceConfigMap);
    }

    public static @NotNull ManagedChannelBuilder<?> configureDefaultRetries(
            @NotNull ManagedChannelBuilder<?> builder,
            @NotNull List<MethodName> methodNames
    ) {
        return configureRetries(builder,
                5,
                createDefaultServiceConfigMap(
                        5,
                        methodNames
                )
        );
    }

    public static @NotNull Map<String, ?> createDefaultServiceConfigMap(
            int maxRetryAttempts,
            List<MethodName> methodNames
    ) {
        Duration initialBackoff = Duration.ofSeconds(1);
        Duration maxBackoff = Duration.ofSeconds(5);
        return ServiceConfig.builder()
                .methodConfig(List.of(
                        MethodConfig.builder()
                                .name(methodNames)
                                .retryPolicy(RetryPolicy.builder()
                                                .maxAttempts(maxRetryAttempts)
                                                .initialBackoff(initialBackoff)
                                                .maxBackoff(maxBackoff)
                                                .backoffMultiplier(getBackoffMultiplier(maxRetryAttempts, initialBackoff, maxBackoff))
//                                        .retryableStatusCodes(List.of(
//                                                Status.Code.UNAVAILABLE,
//                                                Status.Code.ABORTED
//                                        ))
                                                .retryableStatusCodes(List.of(
                                                        Status.Code.UNAVAILABLE
                                                ))
                                                .build()
                                )
//                                        .hedgingPolicy(HedgingPolicy.builder()
//                                                .maxAttempts(maxHedgingAttempts)
//                                                .hedgingDelay(hedgingDelay)
//                                                .nonFatalStatusCodes(List.of(
//                                                        Status.Code.UNAVAILABLE,
//                                                        Status.Code.ABORTED
//                                                ))
//                                                .build()
//                                        )
                                .build()
                ))
                .build()
                .toServiceConfigMap();
    }

    private static double getBackoffMultiplier(int maxRetries, @NotNull Duration initialBackoff, @NotNull Duration maxBackbox) {
        if (maxRetries <= 1 || initialBackoff.compareTo(maxBackbox) >= 0) {
            return 1.0;
        }
        return Math.pow(maxBackbox.toMillis() / 1000.0, 1.0 / maxRetries);
    }


    public static @NotNull ManagedChannelBuilder<?> configureInterceptors(
            @NotNull ManagedChannelBuilder<?> builder,
            ClientInterceptor... clientInterceptors
    ) {
        return builder
                .intercept(clientInterceptors);
    }

    public static @NotNull ManagedChannelBuilder<?> configureDefaultInterceptors(
            @NotNull ManagedChannelBuilder<?> builder,
            @NotNull String applicationName,
            @NotNull String clientName
    ) {
        return builder
                .intercept(getDefaultClientInterceptors(applicationName, clientName));
    }

    public static List<ClientInterceptor> getDefaultClientInterceptors(
            @NotNull String applicationName,
            @NotNull String clientName
    ) {
        return List.of(
                HeaderClientInterceptor.xForwardedFor(),
                HeaderClientInterceptor.requestId(),
                LoggingInterceptor.client(applicationName, clientName),
                MetricsInterceptor.client(clientName),
                TracingInterceptor.client(),
                RidInterceptor.client()
        );
    }


    public record EndpointProperties(
            @NotNull String host,
            int port,
            boolean tls,

            // keepAlive, or nested like keepAlive{time, timeout}
            Duration keepAliveTime,
            Duration keepAliveTimeout,

            // retries, or nested like retries{maxAttempts, maxBackoff}
            int maxAttempts,
            Duration maxBackoff
    ) {
    }

}
