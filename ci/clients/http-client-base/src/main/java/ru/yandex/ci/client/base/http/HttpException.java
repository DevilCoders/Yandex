package ru.yandex.ci.client.base.http;

import javax.annotation.Nullable;

import com.google.common.base.Strings;

public class HttpException extends RuntimeException {

    private static final int MAX_BODY_LENGTH = 1000;

    private final String url;
    private final int httpCode;
    private final int tryNum;
    @Nullable
    private final String body;

    public HttpException(String url, int tryNum, int httpCode, @Nullable String body) {
        super(createMessage(url, tryNum, httpCode, body));
        this.url = url;
        this.httpCode = httpCode;
        this.tryNum = tryNum;
        this.body = body;
    }

    public HttpException(String url, int tryNum, Exception cause) {
        super(cause);
        this.url = url;
        this.httpCode = -1;
        this.tryNum = tryNum;
        this.body = null;
    }

    private static String createMessage(String url, int tryNum, int httpCode, @Nullable String body) {
        StringBuilder message = new StringBuilder();
        message.append("Wrong http code ").append(httpCode)
                .append(" for url '").append(url).append("'");
        if (tryNum > 1) {
            message.append(" after ").append(tryNum).append(" tries");
        }
        message.append(".");
        if (!Strings.isNullOrEmpty(body)) {
            message.append(" Body:\n");
            if (body.length() > MAX_BODY_LENGTH) {
                message.append(body, 0, MAX_BODY_LENGTH).append("...");
            } else {
                message.append(body);
            }
        }
        return message.toString();
    }

    public String getUrl() {
        return url;
    }

    public int getHttpCode() {
        return httpCode;
    }

    public int getTryNum() {
        return tryNum;
    }

    @Nullable
    public String getBody() {
        return body;
    }
}
