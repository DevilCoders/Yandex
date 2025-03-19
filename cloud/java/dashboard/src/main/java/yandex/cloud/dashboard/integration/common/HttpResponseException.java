package yandex.cloud.dashboard.integration.common;

import lombok.Getter;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.util.TextUtils;

/**
 * @author akirakozov
 */
@Getter
public class HttpResponseException extends ClientProtocolException {
    private final int code;
    private final String reason;
    private final String response;

    HttpResponseException(int code, String reason, String response) {
        super(String.format(
                "status code: %d" +
                        (TextUtils.isBlank(reason) ? "" : ", reason: %s") +
                        (TextUtils.isBlank(response) ? "" : ", response: %s"),
                code, reason, response));
        this.code = code;
        this.reason = reason;
        this.response = response;
    }
}
