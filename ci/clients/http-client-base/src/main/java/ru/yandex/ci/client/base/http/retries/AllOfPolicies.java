package ru.yandex.ci.client.base.http.retries;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import okhttp3.Request;
import retrofit2.Response;

import ru.yandex.ci.client.base.http.RetryPolicy;

@RequiredArgsConstructor
public class AllOfPolicies implements RetryPolicy {

    private final RetryPolicy[] policies;

    @Override
    public long canRetry(Request request, @Nullable Response<?> response, int retryNum) {
        // -1 if no retry allowed, 0 - immediate retry, >= 1 - sleep time in millis before retry
        long retrySleepTimeMillis = 0;
        for (RetryPolicy policy : policies) {
            long sleepTimeMillis = policy.canRetry(request, response, retryNum);
            if (sleepTimeMillis < 0) {
                return -1;
            }
            retrySleepTimeMillis = Math.max(retrySleepTimeMillis, sleepTimeMillis);
        }
        return retrySleepTimeMillis;
    }
}
