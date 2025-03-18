package ru.yandex.monlib.metrics.labels.bytes;

import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.LabelAllocator;
import ru.yandex.monlib.metrics.labels.validate.LabelsValidator;


/**
 * @author Sergey Polovko
 */
public final class BytesLabelAllocator implements LabelAllocator {
    private BytesLabelAllocator() {}

    public static final BytesLabelAllocator SELF = new BytesLabelAllocator();

    @Override
    public Label alloc(String key, String value) {
        LabelsValidator.checkKeyValid(key);
        LabelsValidator.checkValueValid(value);
        return new BytesLabel(key, value);
    }
}
