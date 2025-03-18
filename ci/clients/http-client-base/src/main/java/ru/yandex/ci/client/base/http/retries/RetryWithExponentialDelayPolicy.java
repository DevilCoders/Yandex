package ru.yandex.ci.client.base.http.retries;

import java.time.Duration;

import javax.annotation.Nullable;

import okhttp3.Request;
import retrofit2.Response;

import ru.yandex.ci.client.base.http.RetryPolicy;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class RetryWithExponentialDelayPolicy implements RetryPolicy {
    private final Duration startDelay;
    private final Duration maxDelay;
    private final int delayMultiplier;
    private final int retryCount;
    private final int retryNumberStartsMaxDelay;

    public RetryWithExponentialDelayPolicy(int retryCount, Duration startDelay, Duration maxDelay) {
        this(retryCount, startDelay, maxDelay, 2);
    }

    public RetryWithExponentialDelayPolicy(
            int retryCount,
            Duration startDelay,
            Duration maxDelay,
            int delayMultiplier
    ) {
        this.startDelay = startDelay;
        this.retryCount = retryCount;
        this.maxDelay = maxDelay;
        this.delayMultiplier = delayMultiplier;
        if (delayMultiplier <= 1) {
            this.retryNumberStartsMaxDelay = Integer.MAX_VALUE;
            return;
        }

        var delayMillis = startDelay.toMillis();
        var maxDelayMillis = maxDelay.toMillis();
        var retryNumber = 2;
        for (; retryNumber < retryCount; ++retryNumber) {
            if (delayMillis >= maxDelayMillis) {
                break;
            }
            delayMillis *= delayMultiplier;
        }

        retryNumberStartsMaxDelay = retryNumber;
    }

    @Override
    public long canRetry(Request request, @Nullable Response<?> response, int retryNum) {
        return canRetry(retryNum);
    }

    private long canRetry(int retryNum) {
        if (retryNum > this.retryCount) {
            return -1;
        }

        if (retryNum >= retryNumberStartsMaxDelay) {
            return this.maxDelay.toMillis();
        }
        return startDelay.toMillis() * (long) Math.pow(delayMultiplier, retryNum - 2);
    }
}
