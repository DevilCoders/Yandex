package ru.yandex.ci.client.base.http;

import javax.annotation.Nullable;

import okhttp3.Request;

public interface RequestIdProvider {

    @Nullable
    String addRequestId(Request.Builder request);
}
