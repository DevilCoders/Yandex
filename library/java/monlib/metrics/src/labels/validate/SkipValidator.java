package ru.yandex.monlib.metrics.labels.validate;

/**
 * @author Vladimir Gordiychuk
 */
public final class SkipValidator implements LabelValidator {
    public static final SkipValidator SELF = new SkipValidator();

    private SkipValidator() {
    }

    @Override
    public boolean isKeyValid(String key) {
        return true;
    }

    @Override
    public void checkKeyValid(String key) {
        // no op
    }

    @Override
    public boolean isValueValid(String value) {
        return true;
    }

    @Override
    public void checkValueValid(String value) {
        // no op
    }
}
