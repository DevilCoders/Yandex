package ru.yandex.ci.common.grpc;

import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import io.grpc.ManagedChannel;
import io.grpc.MethodDescriptor;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.internal.AbstractManagedChannelImplBuilder;
import io.grpc.netty.NettyChannelBuilder;
import io.grpc.stub.AbstractStub;
import io.netty.channel.ChannelOption;
import lombok.AccessLevel;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

@Slf4j
@RequiredArgsConstructor(access = AccessLevel.PRIVATE)
public class GrpcClientImpl implements GrpcClient {

    private final ManagedChannel channel;
    private final GrpcClientProperties grpcClientProperties;

    @Override
    public <T extends AbstractStub<T>> Supplier<T> buildStub(Function<ManagedChannel, T> constructor) {
        var stub = constructor.apply(channel)
                .withCallCredentials(grpcClientProperties.getCallCredentials());
        return () -> stub.withDeadlineAfter(grpcClientProperties.getDeadlineAfter().toMillis(), TimeUnit.MILLISECONDS);
    }

    @Override
    public void close() throws Exception {
        log.info("Closing gRPC channel for {}", grpcClientProperties.getEndpoint());
        channel.shutdown();
        channel.awaitTermination(grpcClientProperties.getAwaitTermination().toMillis(), TimeUnit.MILLISECONDS);
    }

    public static Builder builder(GrpcClientProperties storageGrpcClientProperties, Class<?> clientType) {
        return new Builder(storageGrpcClientProperties, clientType)
                .withClientLogging();
    }

    @RequiredArgsConstructor
    public static class Builder {
        @Nonnull
        private final GrpcClientProperties properties;

        @Nonnull
        private final Class<?> clientType;

        private String loadBalancing = "round_robin";

        private boolean enableClientLogging;
        private Predicate<MethodDescriptor<?, ?>> logFullResponsePredicate = any -> true;
        @Nonnull
        private Predicate<MethodDescriptor<?, ?>> logEnabledPredicate = any -> true;

        private ManagedChannel buildChannel() {
            var endpoint = properties.getEndpoint();

            AbstractManagedChannelImplBuilder<?> builder;
            if (endpoint.startsWith(GrpcClientProperties.INPROCESS_PREFIX)) {
                log.info("Building in-process gRPC channel for {}", endpoint);
                builder = InProcessChannelBuilder
                        .forName(endpoint.substring(GrpcClientProperties.INPROCESS_PREFIX.length()))
                        .directExecutor();
            } else {
                log.info("Building Netty gRPC channel for {} with {} load balancing", endpoint, loadBalancing);
                builder = NettyChannelBuilder.forTarget(endpoint)
                        .executor(properties.getExecutor())
                        .defaultLoadBalancingPolicy(loadBalancing)
                        .withOption(ChannelOption.CONNECT_TIMEOUT_MILLIS,
                                (int) properties.getConnectTimeout().toMillis());
            }

            builder.maxInboundMessageSize(properties.getMaxInboundMessageSizeInBytes())
                    .userAgent(properties.getUserAgent())
                    .keepAliveTime(properties.getKeepAliveTime().toMillis(), TimeUnit.MILLISECONDS)
                    .keepAliveTimeout(properties.getKeepAliveTimeout().toMillis(), TimeUnit.MILLISECONDS)
                    .enableRetry();

            if (properties.isPlainText()) {
                builder.usePlaintext();
            }

            if (properties.getMaxRetryAttempts() > 0) {
                builder.maxRetryAttempts(properties.getMaxRetryAttempts());
            }

            if (properties.getGrpcServiceConfig() != null) {
                var configs = GrpcServiceConfigUtils.read(properties.getGrpcServiceConfig());
                builder.defaultServiceConfig(configs);
            }

            if (enableClientLogging) {
                builder.intercept(new ClientLoggingInterceptor(
                        clientType.getSimpleName(), logFullResponsePredicate, logEnabledPredicate
                ));
            }

            properties.getInterceptors().forEach(builder::intercept);
            properties.getConfigurers().forEach(configurer -> configurer.accept(builder));

            var channel = builder.build();
            properties.getListeners().forEach(listener -> listener.accept(channel));

            return channel;
        }

        public GrpcClient build() {
            return new GrpcClientImpl(buildChannel(), properties);
        }

        public Builder withoutClientLogging() {
            enableClientLogging = false;
            return this;
        }

        public Builder withClientLogging() {
            enableClientLogging = true;
            return this;
        }

        public Builder excludeLoggingFullResponse(MethodDescriptor<?, ?>... excludeFromLogging) {
            withClientLogging();
            var exclude = Set.of(excludeFromLogging);
            this.logFullResponsePredicate = method -> !exclude.contains(method);
            return this;
        }

        public Builder excludeLogging(MethodDescriptor<?, ?>... ignoreLoggingMethods) {
            withClientLogging();
            var ignore = Set.of(ignoreLoggingMethods);
            this.logEnabledPredicate = method -> !ignore.contains(method);
            return this;
        }

        public Builder withLoadBalancing(String loadBalancing) {
            this.loadBalancing = loadBalancing;
            return this;
        }

    }
}
