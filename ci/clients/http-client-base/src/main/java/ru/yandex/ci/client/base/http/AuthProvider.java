package ru.yandex.ci.client.base.http;

import okhttp3.Request;

public interface AuthProvider {
    void addAuth(Request.Builder request);

    static AuthProvider empty() {
        return request -> {
        };
    }

}
