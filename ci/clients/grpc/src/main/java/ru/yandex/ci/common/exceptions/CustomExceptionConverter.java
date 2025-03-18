package ru.yandex.ci.common.exceptions;

import java.util.Optional;

import io.grpc.Status;

public interface CustomExceptionConverter {
    Optional<Status> convert(Exception e);
}
