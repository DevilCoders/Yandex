package ru.yandex.monlib.metrics.labels.string;

import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.LabelAllocator;
import ru.yandex.monlib.metrics.labels.validate.LabelsValidator;


/**
 * @author Sergey Polovko
 */
public final class StringLabelAllocator implements LabelAllocator {
    private StringLabelAllocator() {}

    public static final StringLabelAllocator SELF = new StringLabelAllocator();

    @Override
    public Label alloc(String key, String value) {
        LabelsValidator.checkKeyValid(key);
        LabelsValidator.checkValueValid(value);
        return new StringLabel(key, value);
    }
}
