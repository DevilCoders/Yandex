package ru.yandex.ci.client.base.http.retries;

import java.util.Set;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import okhttp3.Request;
import retrofit2.Invocation;
import retrofit2.Response;

import ru.yandex.ci.client.base.http.Idempotent;
import ru.yandex.ci.client.base.http.RetryPolicy;

@RequiredArgsConstructor
public class AllowedHttpMethodsPolicy implements RetryPolicy {
    private static final AllowedHttpMethodsPolicy IDEMPOTENT_METHODS =
            new AllowedHttpMethodsPolicy(Set.of("GET", "HEAD", "PUT", "OPTIONS"), true);

    private final Set<String> allowedHttpMethods;
    private final boolean dynamicIdempotent;

    public static AllowedHttpMethodsPolicy defaultIdempotentMethods() {
        return IDEMPOTENT_METHODS;
    }

    @Override
    public long canRetry(Request request, @Nullable Response<?> response, int retryNum) {
        var httpMethod = request.method();
        var idempotent = lookupIdempotent(request);
        if (!allowedHttpMethods.contains(httpMethod)) {
            if (idempotent == null || !idempotent.value()) {
                return -1;
            }
        } else {
            if (idempotent != null && !idempotent.value()) {
                return -1;
            }
        }
        return 0;
    }

    @Nullable
    private Idempotent lookupIdempotent(Request request) {
        if (dynamicIdempotent) {
            var invocation = request.tag(Invocation.class);
            if (invocation != null) {
                return invocation.method().getAnnotation(Idempotent.class);
            }
        }
        return null;
    }
}
