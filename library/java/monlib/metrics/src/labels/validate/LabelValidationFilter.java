package ru.yandex.monlib.metrics.labels.validate;

import javax.annotation.ParametersAreNonnullByDefault;

/**
 * @author Oleg Baryshnikov
 */
@ParametersAreNonnullByDefault
public abstract class LabelValidationFilter {

    public abstract boolean filterKey(String key);

    public abstract boolean filterValue(String value);

    // Skip all metrics for strict validation
    public static LabelValidationFilter ALL = new LabelValidationFilter() {

        @Override
        public boolean filterKey(String key) {
            return true;
        }

        @Override
        public boolean filterValue(String value) {
            return true;
        }
    };

    // Filter invalid label keys/values only
    public static LabelValidationFilter INVALID_ONLY = new LabelValidationFilter() {
        @Override
        public boolean filterKey(String key) {
            return !StrictValidator.SELF.isKeyValid(key);
        }

        @Override
        public boolean filterValue(String value) {
            return !StrictValidator.SELF.isValueValid(value);
        }
    };

    // Filter valid label keys/values only
    public static LabelValidationFilter VALID_ONLY = new LabelValidationFilter() {
        @Override
        public boolean filterKey(String key) {
            return StrictValidator.SELF.isKeyValid(key);
        }

        @Override
        public boolean filterValue(String value) {
            return StrictValidator.SELF.isValueValid(value);
        }
    };
}
