package yandex.cloud.team.integration.idm.core;

import java.io.Serial;
import java.net.http.HttpResponse;

import lombok.Getter;

public class HttpResponseException extends RuntimeException {

    @Serial
    private static final long serialVersionUID = 6180659005915860354L;

    @Getter
    private final HttpResponse<String> response;

    public HttpResponseException(HttpResponse<String> response) {
        super("HTTP " + response.statusCode());
        this.response = response;
    }

}
