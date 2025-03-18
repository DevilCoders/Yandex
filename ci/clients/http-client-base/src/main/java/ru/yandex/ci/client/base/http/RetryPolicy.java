package ru.yandex.ci.client.base.http;

import javax.annotation.Nullable;

import okhttp3.Request;
import retrofit2.Response;

public interface RetryPolicy {

    /**
     * Check if retry available
     *
     * @param request  request to check
     * @param response response to check
     * @param retryNum number of next retry
     * @return -1 if no retry allowed, 0 - immediate retry, >= 1 - sleep time in millis before retry
     */
    long canRetry(Request request, @Nullable Response<?> response, int retryNum);
}
