package ru.yandex.ci.client.base.http;

import java.io.IOException;
import java.time.Duration;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.matchers.Times;
import org.mockserver.model.MediaType;
import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.POST;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockserver.matchers.Times.once;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.HttpStatusCode.INTERNAL_SERVER_ERROR_500;
import static org.mockserver.model.HttpStatusCode.OK_200;

@ExtendWith(MockServerExtension.class)
class RetrofitClientTest {

    private final RetryPolicy shortRetry = RetryPolicies.requireAll(
            RetryPolicies.idempotentMethodsOnly(),
            RetryPolicies.retryWithSleep(7, Duration.ofMillis(10))
    );

    private final MockServerClient server;
    private final HttpClientProperties clientProperties;
    private final String address;
    private TestClient api;

    RetrofitClientTest(MockServerClient server) {
        this.server = server;
        this.address = "http:/" + server.remoteAddress();
        this.clientProperties = HttpClientProperties.builder()
                .endpoint("http:/" + server.remoteAddress())
                .retryPolicy(RetryPolicies.requireAll(
                        RetryPolicies.idempotentMethodsOnly(),
                        RetryPolicies.retryWithSleep(7, Duration.ofMillis(10))
                ))
                .build();
    }

    @BeforeEach
    void setUp() {
        server.reset();
        api = withRetry(null);
    }

    @Test
    void simple() {
        server.when(request("/hello"))
                .respond(
                        response(greeting("Hello, friend!"))
                                .withStatusCode(OK_200.code())
                );

        var response = api.hello();
        assertThat(response.getMessage()).isEqualTo("Hello, friend!");
    }

    @Test
    void serverError() {
        api = withRetry(RetryPolicies.noRetryPolicy());

        server.when(request("/hello"), once()).respond(
                response()
                        .withStatusCode(INTERNAL_SERVER_ERROR_500.code())
                        .withBody("<this is an error>"));

        var errorText = "Wrong http code 500 for url '%s/hello'. Body:\n<this is an error>".formatted(address);
        assertThatThrownBy(() -> api.hello())
                .isInstanceOf(HttpException.class)
                .hasMessage(errorText);
    }

    @Test
    void serverErrorWithRetries() {
        server.when(request("/hello"), once()).respond(
                response()
                        .withStatusCode(INTERNAL_SERVER_ERROR_500.code())
                        .withBody("<this is an error>"));

        var errorText = "Wrong http code 404 for url '%s/hello' after 3 tries.".formatted(address);
        assertThatThrownBy(() -> api.hello())
                .isInstanceOf(HttpException.class)
                .hasMessage(errorText);
    }

    @Test
    void retry() {
        api = withRetry(shortRetry);

        server.when(request("/hello"), Times.exactly(5))
                .respond(response().withStatusCode(INTERNAL_SERVER_ERROR_500.code()));

        server.when(request("/hello"))
                .respond(
                        response(greeting("¡Hola amigo!"))
                                .withContentType(MediaType.JSON_UTF_8)
                                .withStatusCode(OK_200.code())
                );

        assertThat(api.hello().getMessage()).isEqualTo("¡Hola amigo!");

    }

    @Test
    void noRetryPostDefault() {
        api = withRetry(shortRetry);

        server.when(request("/hello-post").withBody("Hello, username"), Times.exactly(5))
                .respond(response().withStatusCode(INTERNAL_SERVER_ERROR_500.code()));

        var errorText = "Wrong http code 500 for url '%s/hello-post'.".formatted(address);
        assertThatThrownBy(() -> api.helloPost("Hello, username"))
                .isInstanceOf(HttpException.class)
                .hasMessage(errorText);

    }

    @Test
    void retryPost() {
        api = withRetry(shortRetry);

        server.when(request("/hello-post"), Times.exactly(5))
                .respond(response().withStatusCode(INTERNAL_SERVER_ERROR_500.code()));

        server.when(request("/hello-post"))
                .respond(
                        response(greeting("¡Hola amigo!"))
                                .withContentType(MediaType.JSON_UTF_8)
                                .withStatusCode(OK_200.code())
                );

        assertThat(api.helloPostIdempotent().getMessage()).isEqualTo("¡Hola amigo!");

    }

    @Test
    void noRetryNonIdempotent() {
        api = withRetry(shortRetry);

        server.when(request("/hello"), Times.exactly(5))
                .respond(response().withStatusCode(INTERNAL_SERVER_ERROR_500.code()));

        var errorText = "Wrong http code 500 for url '%s/hello'.".formatted(address);
        assertThatThrownBy(() -> api.helloNonIdempotent())
                .isInstanceOf(HttpException.class)
                .hasMessage(errorText);
    }

    @Test
    void async() throws IOException {
        server.when(request("/hello"))
                .respond(response(greeting("Привет, друг!"))
                        .withContentType(MediaType.JSON_UTF_8)
                        .withStatusCode(OK_200.code())
                );

        var call = api.helloAsync();
        var response = call.execute();
        assertThat(response.body()).isNotNull();
        assertThat(response.body().getMessage()).isEqualTo("Привет, друг!");
    }

    @Test
    void asyncRetryByHand() throws IOException {
        api = withRetry(shortRetry);

        server.when(request("/hello"), once())
                .respond(response().withStatusCode(INTERNAL_SERVER_ERROR_500.code()));

        server.when(request("/hello"))
                .respond(
                        response(greeting("Salut!"))
                                .withContentType(MediaType.JSON_UTF_8)
                                .withStatusCode(OK_200.code())
                );

        var call = api.helloAsync();
        var response = call.execute();
        assertThat(response.code()).isEqualTo(INTERNAL_SERVER_ERROR_500.code());

        response = call.clone().execute();
        assertThat(response.body()).isNotNull();
        assertThat(response.body().getMessage()).isEqualTo("Salut!");
    }

    private TestClient withRetry(@Nullable RetryPolicy retryPolicy) {
        var policy = Objects.requireNonNullElse(retryPolicy, RetryPolicies.defaultPolicy());
        return RetrofitClient.builder(clientProperties.withRetryPolicy(policy), getClass())
                .build(TestClient.class);
    }

    interface TestClient {
        @GET("/hello")
        Greeting hello();

        @GET("/hello")
        Call<Greeting> helloAsync();

        @POST("/hello-post")
        Greeting helloPost(@Body String content);

        @Idempotent
        @POST("/hello-post")
        Greeting helloPostIdempotent();

        @Idempotent(false)
        @GET("/hello")
        Greeting helloNonIdempotent();
    }

    private static String greeting(String message) {
        return """
                {
                    "message": "%s"
                }
                """.formatted(message);
    }

    public static class Greeting {

        private final String message;

        @JsonCreator
        Greeting(@JsonProperty("message") String message) {
            this.message = message;
        }

        public String getMessage() {
            return message;
        }
    }
}
