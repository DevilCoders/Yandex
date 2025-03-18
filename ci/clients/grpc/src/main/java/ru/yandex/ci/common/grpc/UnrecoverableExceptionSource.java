package ru.yandex.ci.common.grpc;

import io.grpc.StatusRuntimeException;

public interface UnrecoverableExceptionSource {
    RuntimeException newInstance(String message, StatusRuntimeException cause);
}
