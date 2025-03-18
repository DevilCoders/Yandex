package ru.yandex.ci.common.exceptions;

import javax.annotation.Nullable;

import io.grpc.Status;
import io.grpc.StatusRuntimeException;

public class CiStatusRuntimeException extends StatusRuntimeException {
    public CiStatusRuntimeException(Status status, String message) {
        super(buildStatus(status, message, null));
    }

    public CiStatusRuntimeException(Status status, String message, @Nullable Throwable cause) {
        super(buildStatus(status, message, cause));
    }

    private static Status buildStatus(Status status, String message, @Nullable Throwable cause) {
        var resultStatus = status.withDescription(message);
        if (cause != null) {
            resultStatus = resultStatus.withCause(cause);
        }
        return resultStatus;
    }

}
