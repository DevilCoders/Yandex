package ru.yandex.ci.client.base.http;

import java.time.Duration;
import java.util.Set;

import ru.yandex.ci.client.base.http.retries.AllOfPolicies;
import ru.yandex.ci.client.base.http.retries.AllowedHttpMethodsPolicy;
import ru.yandex.ci.client.base.http.retries.NonRetryableResponseCodesPolicy;
import ru.yandex.ci.client.base.http.retries.RetryWithDelayPolicy;
import ru.yandex.ci.client.base.http.retries.RetryWithExponentialDelayPolicy;

public class RetryPolicies {

    public static final RetryPolicy NO_RETRY_POLICY = (request, response, retryNum) -> -1;
    public static final RetryPolicy DEFAULT_RETRY_POLICY = RetryPolicies.requireAll(
            RetryPolicies.idempotentMethodsOnly(),
            RetryPolicies.retryWithSleep(3, Duration.ofSeconds(1))
    );

    private RetryPolicies() {
    }

    public static RetryPolicy requireAll(RetryPolicy... policies) {
        return new AllOfPolicies(policies);
    }

    //

    public static RetryPolicy noRetryPolicy() {
        return NO_RETRY_POLICY;
    }

    public static RetryPolicy defaultPolicy() {
        return DEFAULT_RETRY_POLICY;
    }

    public static RetryPolicy retryWithSleep(int retryCount, Duration sleepDuration) {
        return new RetryWithDelayPolicy(retryCount, sleepDuration);
    }

    public static RetryPolicy retryWithExponentialSleep(int retryCount, Duration startDelay, Duration maxDelay) {
        return new RetryWithExponentialDelayPolicy(retryCount, startDelay, maxDelay);
    }

    public static RetryPolicy idempotentMethodsOnly() {
        return AllowedHttpMethodsPolicy.defaultIdempotentMethods();
    }

    public static RetryPolicy nonRetryableResponseCodes(Set<Integer> responseCodes) {
        return new NonRetryableResponseCodesPolicy(responseCodes);
    }

    //

}
