package yandex.cloud.team.integration.idm;

import yandex.cloud.team.integration.idm.exception.MissingFieldException;
import yandex.cloud.team.integration.idm.exception.MissingParameterException;

/**
 * Static convenience methods to check whether a method was invoked correctly.
 * If the precondition is not met, each method throws an unchecked exception.
 */
public class Validation {

    /**
     * Ensures that a parameter value is not null.
     *
     * @param value a parameter value
     * @param name the name of the parameter
     * @param <T> the type of the parameter
     * @return the validated value
     */
    public static <T> T checkNotNullParameter(T value, String name) {
        if (value == null) {
            throw MissingParameterException.of(name);
        }
        return value;
    }

    /**
     * Ensures that a string parameter value is not null nor empty.
     *
     * @param value a parameter value
     * @param name the name of the parameter
     * @return the validated value
     */
    public static String checkNotNullParameter(String value, String name) {
        if (value == null || value.isEmpty()) {
            throw MissingParameterException.of(name);
        }
        return value;
    }

    /**
     * Ensures that a complex parameter field value is not null.
     *
     * @param value a parameter value
     * @param parameterName the name of the parameter
     * @param fieldName the name of the field
     * @param <T> the type of the field
     * @return the validated value
     */
    public static <T> T checkNotEmptyField(T value, String parameterName, String fieldName) {
        if (value == null) {
            throw MissingFieldException.of(parameterName, fieldName);
        }
        return value;
    }

    /**
     * Ensures that a complex parameter string field value is not null nor empty.
     *
     * @param value a parameter value
     * @param parameterName the name of the parameter
     * @param fieldName the name of the field
     * @return the validated value
     */
    public static String checkNotEmptyField(String value, String parameterName, String fieldName) {
        if (value == null || value.isEmpty()) {
            throw MissingFieldException.of(parameterName, fieldName);
        }
        return value;
    }

}
