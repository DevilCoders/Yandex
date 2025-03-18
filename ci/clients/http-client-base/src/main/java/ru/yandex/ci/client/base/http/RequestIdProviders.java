package ru.yandex.ci.client.base.http;

import java.util.UUID;

import okhttp3.Request;

public class RequestIdProviders {

    public static final String X_REQUEST_ID = "X-Request-Id";

    private RequestIdProviders() {
    }

    public static RequestIdProvider empty() {
        return request -> null;
    }

    public static RequestIdProvider random() {
        return RandomRequestIdProvider.INSTANCE;
    }

    private static class RandomRequestIdProvider implements RequestIdProvider {
        private static final RequestIdProviders.RandomRequestIdProvider INSTANCE =
                new RequestIdProviders.RandomRequestIdProvider();

        @Override
        public String addRequestId(Request.Builder request) {
            var requestId = generateRequestId();
            request.header(X_REQUEST_ID, requestId);
            return requestId;
        }

        private String generateRequestId() {
            return UUID.randomUUID().toString();
        }
    }
}
