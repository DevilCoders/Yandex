package ru.yandex.ci.client.base.http;

import java.io.IOException;
import java.lang.annotation.Annotation;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.Objects;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import retrofit2.Call;
import retrofit2.CallAdapter;
import retrofit2.Response;
import retrofit2.Retrofit;

@Slf4j
class RetrofitClientAdapterFactory extends CallAdapter.Factory {

    private final StatusCodeValidator statusCodeValidator;
    private final RetryPolicy retryPolicy;
    private final CallsMonitor<Call<?>, Response<?>> callsMonitor;

    RetrofitClientAdapterFactory(
            StatusCodeValidator statusCodeValidator,
            RetryPolicy retryPolicy,
            CallsMonitor<Call<?>, Response<?>> callsMonitor
    ) {
        this.statusCodeValidator = statusCodeValidator;
        this.retryPolicy = retryPolicy;
        this.callsMonitor = callsMonitor;
    }

    @Nullable
    @Override
    public CallAdapter<?, ?> get(Type returnType, Annotation[] annotations, Retrofit retrofit) {
        Class<?> rawType = getRawType(returnType);
        CallReturnType callReturnType = CallReturnType.JAVA_TYPE;

        if (rawType.equals(Call.class)) {
            log.warn("Type is not wrapped with default retries: {}", returnType);
            return null; // Unsupported
        }

        if (rawType == Response.class) {
            returnType = ((ParameterizedType) returnType).getActualTypeArguments()[0];
            callReturnType = CallReturnType.RESPONSE;
        }

        return new CallAdapterInternal<>(returnType, callReturnType);
    }

    @RequiredArgsConstructor
    private class CallAdapterInternal<R, T> implements CallAdapter<R, T> {
        private final Type responseType;
        private final CallReturnType callReturnType;

        @Override
        public Type responseType() {
            return responseType;
        }

        @Override
        public T adapt(Call<R> call) {
            //noinspection unchecked
            return (T) execute(call, callReturnType);
        }
    }

    private <R> Object execute(Call<R> call, CallReturnType callReturnType) {
        for (int tryNum = 1; ; tryNum++) {
            Response<R> response = null;
            Exception exception = null;

            var monitoredCall = callsMonitor.start(call, tryNum);
            try {
                response = call.execute();
                monitoredCall.success(response);
            } catch (IOException e) {
                exception = e;
                monitoredCall.failure(e);
            }

            if (exception != null && exception.getCause() instanceof InterruptedException) {
                throw new RuntimeException("Interrupted", exception.getCause());
            }

            if (exception == null && response.isSuccessful() && statusCodeValidator.validate(response.code())) {
                return switch (callReturnType) {
                    case RESPONSE -> response;
                    case JAVA_TYPE -> {
                        R body = response.body();
                        Preconditions.checkState(body != null, "Response body must not return null");
                        yield body;
                    }
                };
            }

            long retrySleepTimeMillis = retryPolicy.canRetry(call.request(), response, tryNum + 1);
            if (retrySleepTimeMillis < 0) {
                String url = call.request().url().toString();
                if (exception != null) {
                    throw new HttpException(url, tryNum, exception);
                } else {
                    throw new HttpException(url, tryNum, response.code(),
                            Objects.requireNonNullElse(monitoredCall.getCachedErrorResponse(), response.message()));
                }
            }
            try {
                //noinspection BusyWait
                Thread.sleep(retrySleepTimeMillis);
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted", e);
            }

            call = call.clone();
        }
    }

    private enum CallReturnType {
        RESPONSE,
        JAVA_TYPE
    }
}
