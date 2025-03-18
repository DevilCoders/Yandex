
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
public final class Labels1 extends Labels {

    private final Label label0;

    public Labels1(Label label0) {
        this.label0 = label0;
    }

    public Labels1(LabelsBuilder builder) {
        this.label0 = builder.at(0);
    }

    @Override
    public int size() {
        return 1;
    }

    @Override
    public boolean isEmpty() {
        return false;
    }

    @Override
    public Label at(int index) {
        checkIndex(index);
        switch (index) {
            case 0: return label0;
        }
        return null; // never get here
    }

    @Override
    public void forEach(Consumer<Label> c) {
        c.accept(label0);
    }

    @Override
    public Stream<Label> stream() {
        return Stream.of(label0);
    }

    @Override
    public int indexByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        if (label0.hasKey(key)) return 0;
        return -1;
    }

    @Override
    public int indexBySameKey(Label label) {
        if (label0.hasSameKey(label)) return 0;
        return -1;
    }

    @Override
    @Nullable
    public Label findByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        if (label0.hasKey(key)) return label0;
        return null;
    }

    @Override
    @Nullable
    public Label findBySameKey(Label label) {
        if (label0.hasSameKey(label)) return label0;
        return null;
    }

    @Override
    public LabelsBuilder toBuilder() {
        return new LabelsBuilder(SortState.SORTED, label0);
    }

    @Override
    public Map<String, String> toMap() {
        Map<String, String> map = new HashMap<>(size());
        map.put(label0.getKey(), label0.getValue());
        return map;
    }

    @Override
    public List<Label> toList() {
        return Collections.singletonList(label0);
    }

    @Override
    public void copyInto(Label[] array) {
        array[0] = label0;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Labels1 rhs = (Labels1) o;
        return label0.equals(rhs.label0);
    }

    @Override
    public int hashCode() {
        return label0.hashCode();
    }

}
