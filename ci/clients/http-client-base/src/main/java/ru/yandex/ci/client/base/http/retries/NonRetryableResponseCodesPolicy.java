package ru.yandex.ci.client.base.http.retries;

import java.util.Set;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import okhttp3.Request;
import retrofit2.Response;

import ru.yandex.ci.client.base.http.RetryPolicy;

@RequiredArgsConstructor
public class NonRetryableResponseCodesPolicy implements RetryPolicy {
    private final Set<Integer> nonRetryableResponseCodes;

    @Override
    public long canRetry(Request request, @Nullable Response<?> response, int retryNum) {
        if (response != null && nonRetryableResponseCodes.contains(response.code())) {
            return -1;
        }
        return 0;
    }
}
