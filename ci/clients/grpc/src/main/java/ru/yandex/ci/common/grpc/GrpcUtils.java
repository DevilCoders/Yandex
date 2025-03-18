package ru.yandex.ci.common.grpc;

import javax.annotation.Nullable;

import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;

import ru.yandex.ci.common.exceptions.CiStatusRuntimeException;

public class GrpcUtils {

    private GrpcUtils() {
    }

    public static StatusRuntimeException notFoundException(String message) {
        return statusRuntimeException(Status.NOT_FOUND, message, null);
    }

    public static StatusRuntimeException notFoundException(Exception cause) {
        return statusRuntimeException(Status.NOT_FOUND, cause.getMessage(), cause);
    }

    public static StatusRuntimeException invalidArgumentException(String message) {
        return statusRuntimeException(Status.INVALID_ARGUMENT, message, null);
    }

    public static StatusRuntimeException invalidArgumentException(Exception cause) {
        return statusRuntimeException(Status.INVALID_ARGUMENT, cause.getMessage(), cause);
    }

    public static StatusRuntimeException permissionDeniedException(String message) {
        return statusRuntimeException(Status.PERMISSION_DENIED, message, null);
    }

    public static StatusRuntimeException permissionDeniedException(Exception cause) {
        return statusRuntimeException(Status.PERMISSION_DENIED, cause.getMessage(), cause);
    }

    public static StatusRuntimeException failedPreconditionException(String message) {
        return statusRuntimeException(Status.FAILED_PRECONDITION, message, null);
    }

    public static StatusRuntimeException failedPreconditionException(Exception cause) {
        return statusRuntimeException(Status.FAILED_PRECONDITION, cause.getMessage(), cause);
    }

    public static StatusRuntimeException resourceExhausted(String message) {
        return statusRuntimeException(Status.RESOURCE_EXHAUSTED, message, null);
    }

    public static StatusRuntimeException internalError(String message) {
        return statusRuntimeException(Status.INTERNAL, message, null);
    }

    public static StatusRuntimeException internalError(Exception cause) {
        return statusRuntimeException(Status.INTERNAL, cause.getMessage(), cause);
    }

    public static StatusRuntimeException unauthenticated(String message) {
        return statusRuntimeException(Status.UNAUTHENTICATED, message, null);
    }

    private static StatusRuntimeException statusRuntimeException(
            Status status,
            String message,
            @Nullable Exception cause
    ) {
        return new CiStatusRuntimeException(status, message, cause);
    }

    public static <Request extends Message> RuntimeException wrapRepeatableException(
            StatusRuntimeException e,
            Request request,
            String errorMessage,
            UnrecoverableExceptionSource exceptionSource
    ) {
        var code = e.getStatus().getCode();
        return switch (code) {
            case ALREADY_EXISTS,
                    FAILED_PRECONDITION,
                    INVALID_ARGUMENT,
                    NOT_FOUND,
                    PERMISSION_DENIED,
                    UNAUTHENTICATED -> {
                var msg = errorMessage + ": " + TextFormat.shortDebugString(request);
                yield exceptionSource.newInstance(msg, e);
            }
            default -> e;
        };
    }
}
