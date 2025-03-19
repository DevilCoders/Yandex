package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.iam.exception.BadRequestException;

final class ProtoRequests {

    static @Nullable String optionalField(
            @Nullable String value,
            @NotNull String fieldName
    ) {
        if (value == null || value.isEmpty()) {
            return null;
        }
        return value;
    }

    static long optionalField(
            long value,
            @NotNull String fieldName
    ) {
        return value;
    }

    static long optionalField(
            long value,
            long defaultValue,
            @NotNull String fieldName
    ) {
        return value != 0 ? value : defaultValue;
    }

    static boolean optionalField(
            boolean value,
            @NotNull String fieldName
    ) {
        return value;
    }


    static @NotNull String requiredField(
            @Nullable String value,
            @NotNull String fieldName
    ) {
        if (value == null || value.isEmpty()) {
            throw new RequiredFieldException(String.format("%s is required", fieldName));
        }
        return value;
    }

    static long requiredField(
            long value,
            @NotNull String fieldName
    ) {
        if (value == 0L) {
            throw new RequiredFieldException(String.format("%s is required", fieldName));
        }
        return value;
    }


    static void unsupportedField(
            boolean hasValue,
            @NotNull String fieldName
    ) {
        if (hasValue) {
            throw new UnsupportedFieldException(String.format("%s is not supported", fieldName));
        }
    }


    private ProtoRequests() {
    }


    private static class RequiredFieldException extends BadRequestException {

        RequiredFieldException(String message) {
            super(message);
        }

    }

    private static class UnsupportedFieldException extends BadRequestException {

        UnsupportedFieldException(String message) {
            super(message);
        }

    }

}
