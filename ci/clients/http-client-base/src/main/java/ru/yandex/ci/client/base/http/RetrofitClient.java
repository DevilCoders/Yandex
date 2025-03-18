package ru.yandex.ci.client.base.http;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.function.Consumer;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import com.fasterxml.jackson.datatype.jdk8.Jdk8Module;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import okhttp3.Request;
import org.asynchttpclient.AsyncHttpClient;
import org.asynchttpclient.DefaultAsyncHttpClient;
import org.asynchttpclient.DefaultAsyncHttpClientConfig;
import org.asynchttpclient.extras.retrofit.AsyncHttpClientCallFactory;
import retrofit2.Call;
import retrofit2.Response;
import retrofit2.Retrofit;
import retrofit2.converter.jackson.JacksonConverterFactory;

import ru.yandex.ci.client.base.http.retrofit.QueryValueEnumConverterFactory;

public class RetrofitClient {
    private static final ConcurrentMap<AsyncClientConfiguration, AsyncHttpClient> CLIENT_CACHE =
            new ConcurrentHashMap<>();

    private RetrofitClient() {
    }

    public static Builder builder(HttpClientProperties clientProperties, Class<?> clientType) {
        return new Builder(clientProperties, clientType);
    }

    @RequiredArgsConstructor
    public static class Builder {
        @Nonnull
        private final HttpClientProperties clientProperties;
        @Nonnull
        private final Class<?> clientType;

        @Nullable
        private AuthProvider authProvider;
        @Nullable
        private StatusCodeValidator statusCodeValidator;
        @Nullable
        private ObjectMapper objectMapper;
        @Nullable
        private CallsMonitor<Call<?>, Response<?>> callsMonitor;
        @Nullable
        private RequestIdProvider requestIdProvider;
        @Nullable
        private AsyncHttpClient asyncHttpClient;

        private final List<Consumer<Request.Builder>> requestMappers = new ArrayList<>(
                List.of(builder -> builder.header("Accept", "application/json"))
        );

        public <T> T build(Class<T> service) {
            if (asyncHttpClient == null) {
                asyncHttpClient = buildHttpClient(clientProperties);
            }

            // В текущей реализации Async HTTP Client-а нет никакого смысла подписываться
            // на методы requestStart - они будут такими же за исключением времени на
            // кодирование/декодирование java объекта
            AsyncHttpClientCallFactory callFactory = AsyncHttpClientCallFactory.builder()
                    .httpClient(asyncHttpClient)
                    .callCustomizer(RetrofitLoggingCalls.getAsyncCallCustomizer())
                    .build();

            if (authProvider == null) {
                authProvider = Objects.requireNonNullElse(clientProperties.getAuthProvider(), AuthProvider.empty());
            }
            if (statusCodeValidator == null) {
                statusCodeValidator = StatusCodeValidators.http2xxStatusCodeValidator();
            }
            if (objectMapper == null) {
                objectMapper = defaultObjectMapper();
            }
            if (callsMonitor == null) {
                var monitorSource = Objects.requireNonNullElse(
                        clientProperties.getCallsMonitorSource(),
                        CallsMonitorSource.simple()
                );
                var clientName = clientType.getSimpleName() +
                        Objects.requireNonNullElse(clientProperties.getClientNameSuffix(), "");
                callsMonitor = monitorSource.buildRetrofitMonitor(clientName);
            }
            if (requestIdProvider == null) {
                requestIdProvider = RequestIdProviders.random();
            }

            requestMappers.add(authProvider::addAuth);
            requestMappers.add(requestIdProvider::addRequestId);

            var clientName = callsMonitor.getClientName();
            var clientAdapterFactory = new RetrofitClientAdapterFactory(
                    statusCodeValidator, clientProperties.getRetryPolicy(), callsMonitor);

            var mappers = requestMappers.toArray(Consumer[]::new);
            var retrofit = new Retrofit.Builder()
                    .baseUrl(clientProperties.getEndpoint())
                    .addCallAdapterFactory(clientAdapterFactory)
                    .addConverterFactory(JacksonConverterFactory.create(objectMapper))
                    .addConverterFactory(new QueryValueEnumConverterFactory())
                    .validateEagerly(true)
                    .callFactory(request -> {
                        var builder = request.newBuilder();
                        for (var mapper : mappers) {
                            //noinspection unchecked
                            mapper.accept(builder);
                        }
                        RetrofitLoggingCalls.configureContext(clientName, request, builder);
                        return callFactory.newCall(builder.build());
                    })
                    .build();

            return retrofit.create(service);
        }

        public Builder statusCodeValidator(@Nullable StatusCodeValidator statusCodeValidator) {
            this.statusCodeValidator = statusCodeValidator;
            return this;
        }

        public Builder objectMapper(@Nullable ObjectMapper objectMapper) {
            this.objectMapper = objectMapper;
            return this;
        }

        public Builder snakeCaseNaming() {
            getObjectMapper().setPropertyNamingStrategy(PropertyNamingStrategies.SNAKE_CASE);
            return this;
        }

        public Builder failOnMissingCreatorProperties() {
            getObjectMapper().enable(DeserializationFeature.FAIL_ON_MISSING_CREATOR_PROPERTIES);
            return this;
        }

        @SuppressWarnings("deprecation")
        public Builder withNonNumericNumbers() {
            getObjectMapper().enable(JsonParser.Feature.ALLOW_NON_NUMERIC_NUMBERS);
            return this;
        }

        public Builder requestIdProvider(@Nullable RequestIdProvider requestIdProvider) {
            this.requestIdProvider = requestIdProvider;
            return this;
        }

        public Builder addRequestMapper(Consumer<Request.Builder> mapper) {
            this.requestMappers.add(mapper);
            return this;
        }

        public ObjectMapper getObjectMapper() {
            if (objectMapper == null) {
                objectMapper(defaultObjectMapper());
            }
            return objectMapper;
        }

        public static ObjectMapper defaultObjectMapper() {
            return new ObjectMapper()
                    .registerModule(new Jdk8Module())
                    .registerModule(new JavaTimeModule())
                    .enable(DeserializationFeature.READ_UNKNOWN_ENUM_VALUES_USING_DEFAULT_VALUE)
                    .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES)
                    .setSerializationInclusion(JsonInclude.Include.NON_NULL);
        }
    }

    private static AsyncHttpClient buildHttpClient(HttpClientProperties properties) {
        // Reuse client
        return CLIENT_CACHE.computeIfAbsent(
                new AsyncClientConfiguration(
                        properties.getRequestTimeout(),
                        properties.getConnectionTimeout(),
                        properties.getFollowRedirects()
                ),
                RetrofitClient::buildHttpClient
        );
    }

    private static AsyncHttpClient buildHttpClient(AsyncClientConfiguration cfg) {
        var requestTimeout = cfg.getRequestTimeout();
        var connectionTimeout = cfg.getConnectionTimeout();

        var asyncHttpClientConfig = new DefaultAsyncHttpClientConfig.Builder()
                .setConnectTimeout((int) connectionTimeout.toMillis())
                .setRequestTimeout((int) requestTimeout.toMillis())
                .addRequestFilter(RetrofitLoggingCalls.getRequestFilter());

        if (cfg.getFollowRedirects() != null) {
            // Do not change default behavior if not configured
            asyncHttpClientConfig.setFollowRedirect(cfg.getFollowRedirects());
        }

        return new DefaultAsyncHttpClient(asyncHttpClientConfig.build());
    }

    @Value
    private static class AsyncClientConfiguration {
        @Nonnull
        Duration requestTimeout;
        @Nonnull
        Duration connectionTimeout;
        @Nullable
        Boolean followRedirects;
    }

}
