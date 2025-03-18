package ru.yandex.ci.common.exceptions;

import java.util.NoSuchElementException;
import java.util.Optional;

import io.grpc.Status;

public class DefaultExceptionConverter implements CustomExceptionConverter {
    private DefaultExceptionConverter() {

    }

    public static DefaultExceptionConverter instance() {
        return new DefaultExceptionConverter();
    }

    @Override
    public Optional<Status> convert(Exception e) {
        if (e instanceof NoSuchElementException) {
            return Optional.of(Status.NOT_FOUND.withDescription(e.getMessage()));
        } else if (e instanceof IllegalArgumentException) {
            return Optional.of(Status.INVALID_ARGUMENT.withDescription(e.getMessage()));
        }

        return Optional.empty();
    }
}
