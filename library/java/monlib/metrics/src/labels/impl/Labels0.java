
package ru.yandex.monlib.metrics.labels.impl;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import java.util.stream.Stream;

import javax.annotation.Nullable;
import javax.annotation.ParametersAreNonnullByDefault;

import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.labels.LabelsBuilder;
import ru.yandex.monlib.metrics.labels.LabelsBuilder.SortState;
import ru.yandex.monlib.metrics.labels.validate.LabelsValidator;

/**
 * This class is automatically generated. Do not edit it!
 */
@ParametersAreNonnullByDefault
public final class Labels0 extends Labels {

    public static final Labels0 SELF = new Labels0();

    @Override
    public int size() {
        return 0;
    }

    @Override
    public boolean isEmpty() {
        return true;
    }

    @Override
    public Label at(int index) {
        checkIndex(index);
        return null; // never get here
    }

    @Override
    public void forEach(Consumer<Label> c) {
    }

    @Override
    public Stream<Label> stream() {
        return Stream.empty();
    }

    @Override
    public int indexByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        return -1;
    }

    @Override
    public int indexBySameKey(Label label) {
        return -1;
    }

    @Override
    @Nullable
    public Label findByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        return null;
    }

    @Override
    @Nullable
    public Label findBySameKey(Label label) {
        return null;
    }

    @Override
    public LabelsBuilder toBuilder() {
        return new LabelsBuilder(SortState.SORTED);
    }

    @Override
    public Map<String, String> toMap() {
        return new HashMap<>(0);
    }

    @Override
    public List<Label> toList() {
        return Collections.emptyList();
    }

    @Override
    public void copyInto(Label[] array) {
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        return o != null && getClass() == o.getClass();
    }

    @Override
    public int hashCode() {
        return 31;
    }

}
