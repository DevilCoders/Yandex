package yandex.cloud.ti.abcd.adapter;

import java.util.Objects;

import lombok.experimental.UtilityClass;

@UtilityClass
public class Validator {

    public static void checkMatch(
            long expected,
            long actual,
            String fieldName
    ) {
        if (expected != actual) {
            throw ParameterMismatchException.of(expected, actual, fieldName);
        }
    }

    public static void checkMatch(
            Object expected,
            Object actual,
            String fieldName
    ) {
        if (!Objects.equals(expected, actual)) {
            throw ParameterMismatchException.of(expected, actual, fieldName);
        }
    }

    public static void requiredField(
            long value,
            String parameterName
    ) {
        if (value == 0L) {
            throw MissingRequiredParameterException.of(parameterName);
        }
    }

    public static void requiredField(
            String value,
            String parameterName
    ) {
        if (value == null || value.isEmpty()) {
            throw MissingRequiredParameterException.of(parameterName);
        }
    }

}
