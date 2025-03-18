package ru.yandex.monlib.metrics.labels;

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.monlib.metrics.labels.bytes.BytesLabelAllocator;
import ru.yandex.monlib.metrics.labels.impl.Labels0;
import ru.yandex.monlib.metrics.labels.impl.Labels1;
import ru.yandex.monlib.metrics.labels.string.StringLabelAllocator;
import ru.yandex.monlib.metrics.labels.validate.LabelsValidator;


/**
 * @author Sergey Polovko
 */
class LabelsStaticPart {

    private static final Logger logger = LoggerFactory.getLogger(Labels.class);

    public static final int MAX_LABELS_COUNT = 16;

    public static final LabelAllocator allocator;
    static {
        String preferredAlloc = System.getProperty("ru.yandex.solomon.LabelAllocator", "string");
        switch (preferredAlloc.toLowerCase()) {
            case "string":
                allocator = StringLabelAllocator.SELF;
                break;
            case "bytes":
                allocator = BytesLabelAllocator.SELF;
                break;
            default:
                allocator = StringLabelAllocator.SELF;
        }

        if (logger.isInfoEnabled()) {
            logger.info("-Dru.yandex.solomon.LabelAllocator=" + preferredAlloc);
        }
    }

    // - factory shortcuts -

    public static LabelsBuilder builder() {
        return builder(MAX_LABELS_COUNT);
    }

    public static LabelsBuilder builder(int capacity) {
        return new LabelsBuilder(capacity);
    }

    public static LabelsBuilder builder(int capacity, LabelAllocator allocator) {
        return new LabelsBuilder(capacity, allocator);
    }

    public static LabelsBuilder builder(Label... labels) {
        return new LabelsBuilder(LabelsBuilder.SortState.MAYBE_NOT_SORTED, labels);
    }

    public static Labels empty() {
        return Labels0.SELF;
    }

    public static Labels of() {
        return empty();
    }

    public static Labels of(String key, String value) {
        return new Labels1(allocator.alloc(key, value));
    }

    public static Labels of(
        String key1, String value1,
        String key2, String value2)
    {
        return new LabelsBuilder(2)
            .add(key1, value1)
            .add(key2, value2)
            .build();
    }

    public static Labels of(
        String key1, String value1,
        String key2, String value2,
        String key3, String value3)
    {
        return new LabelsBuilder(3)
            .add(key1, value1)
            .add(key2, value2)
            .add(key3, value3)
            .build();
    }

    public static Labels of(
        String key1, String value1,
        String key2, String value2,
        String key3, String value3,
        String key4, String value4)
    {
        return new LabelsBuilder(4)
            .add(key1, value1)
            .add(key2, value2)
            .add(key3, value3)
            .add(key4, value4)
            .build();
    }

    public static Labels of(
        String key1, String value1,
        String key2, String value2,
        String key3, String value3,
        String key4, String value4,
        String key5, String value5)
    {
        return new LabelsBuilder(5)
            .add(key1, value1)
            .add(key2, value2)
            .add(key3, value3)
            .add(key4, value4)
            .add(key5, value5)
            .build();
    }

    public static Labels of(Label label) {
        return new Labels1(label);
    }

    public static Labels of(Label... labels) {
        if (labels.length == 0) {
            return Labels0.SELF;
        }

        LabelsBuilder builder = new LabelsBuilder(labels.length);
        // we must to add each label separately to guaranty key uniqueness
        for (Label label : labels) {
            builder.add(label);
        }
        return builder.build();
    }

    public static Labels of(Map<String, String> labels) {
        LabelsValidator.checkCountValid(labels.size());
        return new LabelsBuilder(labels.size())
            .addAll(labels)
            .build();
    }
}
