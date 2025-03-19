package yandex.cloud.ti.http.client;

import java.io.Serial;
import java.net.URI;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.ServerException;

/**
 * Represents an error condition occurred during a request to an external API.
 */
public class HttpClientException extends ServerException {

    @Serial
    private static final long serialVersionUID = -1289673081202580617L;

    public HttpClientException(
        @NotNull String message
    ) {
        super(message);
    }

    private HttpClientException(
        @NotNull String message,
        @NotNull Exception ex
    ) {
        super(message, ex);
    }

    public static @NotNull HttpClientException cancelled(
        @NotNull URI uri,
        @NotNull Exception ex
    ) {
        return new HttpClientException(
            String.format("Call to '%s' cancelled", uri), ex);
    }

    public static @NotNull HttpClientException forStatusCode(
        @NotNull URI uri,
        int statusCode
    ) {
        return new HttpClientException(
            String.format("Bad response from '%s', status code: %d", uri, statusCode));
    }

    public static @NotNull HttpClientException ioError(
        @NotNull URI uri,
        @NotNull Exception ex
    ) {
        return new HttpClientException(
            String.format("IO error during call to '%s'", uri), ex);
    }

    public static @NotNull HttpClientException malformedResponse(
        @NotNull URI uri,
        @NotNull Exception ex
    ) {
        return new HttpClientException(
            String.format("Malformed response from  %s", uri), ex);
    }

}
