package ru.yandex.monlib.metrics.labels.validate;

import javax.annotation.ParametersAreNonnullByDefault;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.monlib.metrics.labels.Labels;


/**
 * @author Sergey Polovko
 */
@ParametersAreNonnullByDefault
public final class LabelsValidator {
    private static final Logger logger = LoggerFactory.getLogger(LabelsValidator.class);

    private LabelsValidator() {}

    public static final LabelValidator validator;
    static {
        String preferredValidator = System.getProperty("ru.yandex.solomon.LabelValidator", "strict");
        switch (preferredValidator.toLowerCase()) {
            case "strict":
                validator = StrictValidator.SELF;
                break;
            case "skip":
                validator = SkipValidator.SELF;
                break;
            default:
                validator = StrictValidator.SELF;
        }

        if (logger.isInfoEnabled()) {
            logger.info("-Dru.yandex.solomon.LabelValidator=" + preferredValidator);
        }
    }

    // -- count --

    public static boolean isCountValid(int count) {
        return count >= 0 && count <= Labels.MAX_LABELS_COUNT;
    }

    public static void checkCountValid(int count) {
        if (!isCountValid(count)) {
            throw new TooManyLabelsException("count: " + count);
        }
    }

    // -- key --

    public static boolean isKeyValid(String key) {
        return validator.isKeyValid(key);
    }

    public static void checkKeyValid(String key) {
        validator.checkKeyValid(key);
    }

    // -- value --

    public static boolean isValueValid(String value) {
        return validator.isValueValid(value);
    }

    public static void checkValueValid(String value) {
        validator.checkValueValid(value);
    }
}
