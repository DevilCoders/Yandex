package yandex.cloud.ti.http.client;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
import java.util.function.Supplier;

import lombok.extern.log4j.Log4j2;
import org.apache.http.HttpStatus;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.ClientConfig;
import yandex.cloud.http.Headers;
import yandex.cloud.util.Json;

@Log4j2
public class AbstractHttpClient {

    private final String apiPrefix;

    private final String idempotencyKey;

    protected final ClientConfig endpoint;

    private final HttpClient httpClient;

    protected final String userAgent;

    protected final Supplier<String> token;

    public AbstractHttpClient(
            @NotNull ClientConfig endpoint,
            @NotNull String apiPrefix,
            @NotNull String userAgent,
            Supplier<String> token,
            String idempotencyKey) {
        this.userAgent = userAgent;
        this.endpoint = endpoint;
        this.token = token;
        this.apiPrefix = apiPrefix;
        this.idempotencyKey = idempotencyKey;

        httpClient = HttpClient.newBuilder()
            .followRedirects(HttpClient.Redirect.NEVER)
            .connectTimeout(endpoint.getTimeout())
            .build();
    }

    private URI createUri(
        String method,
        String query
    ) {
        var scheme = endpoint.isTls() ? "https" : "http";
        try {
            return new URI(
                    scheme,
                    null,
                    endpoint.getHost(),
                    endpoint.getPort(),
                    method == null ? apiPrefix : apiPrefix + method,  // todo: move apiPrefix into calls
                    query,
                    null
            );
        } catch (URISyntaxException e) {
            throw new IllegalArgumentException(e);
        }
    }

    protected <IN, OUT> OUT sendRequest(
        String path,
        String query,
        IN data,
        Class<OUT> responseType
    ) {
        var uri = createUri(path, query);
        var request = HttpRequest.newBuilder()
            .uri(uri)
            .setHeader(Headers.USER_AGENT, userAgent)
            .timeout(endpoint.getTimeout())
            .version(HttpClient.Version.HTTP_1_1);

        if (idempotencyKey != null) {
            request = request
                .setHeader(Headers.IDEMPOTENCY_KEY, stringToUuid(idempotencyKey));
        }

        if (data != null) {
            request = request
                .POST(HttpRequest.BodyPublishers.ofString(Json.toJson(data), StandardCharsets.UTF_8))
                .setHeader(Headers.CONTENT_TYPE, "application/json");
        }

        HttpResponse<String> rawResponse;
        try {
            rawResponse = httpClient.send(prepareRequest(request).build(), HttpResponse.BodyHandlers.ofString());
        } catch (InterruptedException | InterruptedIOException ex) {
            Thread.currentThread().interrupt();
            log.debug("{} interrupted", uri, ex);
            throw HttpClientException.cancelled(uri, ex);
        } catch (IOException ex) {
            log.debug("{} failed", uri, ex);
            throw HttpClientException.ioError(uri, ex);
        }

        if (!isSucceeded(rawResponse.statusCode())) {
            log.debug("{} failed: {}", uri, rawResponse.body());
            throw HttpClientException.forStatusCode(uri, rawResponse.statusCode());
        }

        log.trace("{} => {}", uri, rawResponse.body());

        if (responseType == null) {
            return null;
        }

        try {
            var type = Json.mapper.getTypeFactory().constructType(responseType);
            return Json.mapper.readerFor(type).readValue(rawResponse.body());
        } catch (IOException ex) {
            log.debug("{} failed to parse response: {}", uri, rawResponse.body(), ex);
            throw HttpClientException.malformedResponse(uri, ex);
        }
    }

    protected boolean isSucceeded(
        int statusCode
    ) {
        return switch (statusCode) {
            case HttpStatus.SC_OK,
                HttpStatus.SC_CREATED,
                HttpStatus.SC_NO_CONTENT -> true;
            default -> false;
        };
    }

    protected HttpRequest.Builder prepareRequest(
        @NotNull HttpRequest.Builder builder
    ) {
        return builder
            .setHeader(Headers.AUTHORIZATION, "Bearer " + token.get());
    }

    /**
     * Some API wants the idempotence key to be a UUID.
     *
     * @param idempotencyKey any string
     * @return the {@code idempotencyKey} hash in uuid-compatible string
     */
    public static String stringToUuid(
        @NotNull String idempotencyKey
    ) {
        return UUID.nameUUIDFromBytes(idempotencyKey.getBytes(StandardCharsets.UTF_8)).toString();
    }

}
