package ru.yandex.ci.client.base.http.retries;

import java.time.Duration;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import okhttp3.Request;
import retrofit2.Response;

import ru.yandex.ci.client.base.http.RetryPolicy;

@RequiredArgsConstructor
public class RetryWithDelayPolicy implements RetryPolicy {
    private final int retryCount;
    private final Duration sleepDuration;

    @Override
    public long canRetry(Request request, @Nullable Response<?> response, int retryNum) {
        return (retryNum <= retryCount) ? sleepDuration.toMillis() : -1;
    }
}
