package ru.yandex.ci.core.exceptions;

import java.util.NoSuchElementException;
import java.util.Optional;

import io.grpc.Status;

import ru.yandex.ci.common.exceptions.CustomExceptionConverter;

public class CiExceptionsConverter implements CustomExceptionConverter {
    private CiExceptionsConverter() {

    }

    public static CiExceptionsConverter instance() {
        return new CiExceptionsConverter();
    }

    @Override
    public Optional<Status> convert(Exception e) {
        if (e instanceof NoSuchElementException || e instanceof CiNotFoundException) {
            return Optional.of(Status.NOT_FOUND.withDescription(e.getMessage()));
        } else if (e instanceof IllegalArgumentException) {
            return Optional.of(Status.INVALID_ARGUMENT.withDescription(e.getMessage()));
        } else if (e instanceof CiFailedPreconditionException) {
            return Optional.of(Status.FAILED_PRECONDITION.withDescription(e.getMessage()));
        } else if (e instanceof CiDuplicateException) {
            return Optional.of(Status.ALREADY_EXISTS.withDescription(e.getMessage()));
        }

        return Optional.empty();
    }
}
