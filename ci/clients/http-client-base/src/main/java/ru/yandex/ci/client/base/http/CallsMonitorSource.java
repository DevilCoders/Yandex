package ru.yandex.ci.client.base.http;

import retrofit2.Call;
import retrofit2.Response;

public interface CallsMonitorSource {

    default CallsMonitor<Call<?>, Response<?>> buildRetrofitMonitor(Class<?> clientClass) {
        return buildRetrofitMonitor(clientClass.getSimpleName());
    }

    CallsMonitor<Call<?>, Response<?>> buildRetrofitMonitor(String clientName);

    // Log each request with default logger
    static CallsMonitorSource simple() {
        return RetrofitLoggingCalls::create;
    }
}
