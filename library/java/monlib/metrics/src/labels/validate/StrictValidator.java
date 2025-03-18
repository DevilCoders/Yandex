package ru.yandex.monlib.metrics.labels.validate;

import javax.annotation.Nullable;

/**
 * @author Sergey Polovko
 */
public class StrictValidator implements LabelValidator {
    public static final StrictValidator SELF = new StrictValidator();

    private static final CharsPattern KEY_PATTERN = new CharsPattern.Builder()
        .maxLen(32)
        .add('a', 'z')
        .add('A', 'Z')
        .addRest('0', '9')
        .addRest("_.")
        .build();

    private static final CharsPattern VALUE_PATTERN = new CharsPattern.Builder()
        .maxLen(200)
        .add(' ', '~')
        .ignore("*?|\"'`\\")
        .build();

    // -- key --

    @Override
    public boolean isKeyValid(String key) {
        return validateKey(key) == null;
    }

    @Override
    public void checkKeyValid(String key) {
        final String message = validateKey(key);
        if (message != null) {
            throw new InvalidLabelException(message);
        }
    }

    @Nullable
    public static String validateKey(String key) {
        return validate("key", key, KEY_PATTERN);
    }

    // -- value --

    @Override
    public boolean isValueValid(String value) {
        return validateValue(value) == null;
    }

    @Override
    public void checkValueValid(String value) {
        final String message = validateValue(value);
        if (message != null) {
            throw new InvalidLabelException(message);
        }
    }

    @Nullable
    public static String validateValue(String value) {
        return validate("value", value, VALUE_PATTERN);
    }

    private static String validate(String name, String content, CharsPattern pattern) {
        final int len = content.length();
        if (len == 0) {
            return name + " is empty";
        }
        if (!pattern.isValidLen(len)) {
            return String.format(name + " '%s' is too long, maximum is %d", content, pattern.getMaxLen());
        }
        if (content.equals("-")) {
            return "'-' is invalid label value";
        }
        if (!pattern.isValidFirst(content.charAt(0))) {
            return String.format(name + " '%s' is not starts with permitted character", content);
        }
        for (int i = 1; i < len; i++) {
            if (!pattern.isValidRest(content.charAt(i))) {
                return String.format(name + " '%s' contains non permitted character at position %d", content, i);
            }
        }
        return null;
    }
}
