package ru.yandex.monlib.metrics.labels.validate;

import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;


/**
 * @author Vladimir Gordiychuk
 */
public interface LabelValidator {

    default boolean isValid(Labels labels) {
        for (int i = 0; i < labels.size(); i++) {
            if (!isValid(labels.at(i))) {
                return false;
            }
        }
        return true;
    }

    default void checkValid(Labels labels) {
        labels.forEach(this::checkValid);
    }

    default boolean isValid(Label label) {
        return isKeyValid(label.getKey()) && isValueValid(label.getValue());
    }

    default void checkValid(Label label) {
        checkKeyValid(label.getKey());
        checkValueValid(label.getValue());
    }

    boolean isKeyValid(String key);

    /**
     * @throws InvalidLabelException in case not valid label key
     */
    void checkKeyValid(String key);

    boolean isValueValid(String value);

    /**
     * @throws InvalidLabelException in case not valid label value
     */
    void checkValueValid(String value);
}
