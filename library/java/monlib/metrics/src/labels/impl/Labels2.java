
package ru.yandex.monlib.metrics.labels.impl;

import java.util.Arrays;
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
public final class Labels2 extends Labels {

    private final Label label0;
    private final Label label1;

    public Labels2(Label label0, Label label1) {
        this.label0 = label0;
        this.label1 = label1;
    }

    public Labels2(LabelsBuilder builder) {
        this.label0 = builder.at(0);
        this.label1 = builder.at(1);
    }

    @Override
    public int size() {
        return 2;
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
            case 1: return label1;
        }
        return null; // never get here
    }

    @Override
    public void forEach(Consumer<Label> c) {
        c.accept(label0);
        c.accept(label1);
    }

    @Override
    public Stream<Label> stream() {
        return Stream.of(label0, label1);
    }

    @Override
    public int indexByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        if (label0.hasKey(key)) return 0;
        if (label1.hasKey(key)) return 1;
        return -1;
    }

    @Override
    public int indexBySameKey(Label label) {
        if (label0.hasSameKey(label)) return 0;
        if (label1.hasSameKey(label)) return 1;
        return -1;
    }

    @Override
    @Nullable
    public Label findByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        if (label0.hasKey(key)) return label0;
        if (label1.hasKey(key)) return label1;
        return null;
    }

    @Override
    @Nullable
    public Label findBySameKey(Label label) {
        if (label0.hasSameKey(label)) return label0;
        if (label1.hasSameKey(label)) return label1;
        return null;
    }

    @Override
    public LabelsBuilder toBuilder() {
        return new LabelsBuilder(SortState.SORTED, label0, label1);
    }

    @Override
    public Map<String, String> toMap() {
        Map<String, String> map = new HashMap<>(size());
        map.put(label0.getKey(), label0.getValue());
        map.put(label1.getKey(), label1.getValue());
        return map;
    }

    @Override
    public List<Label> toList() {
        return Arrays.asList(label0, label1);
    }

    @Override
    public void copyInto(Label[] array) {
        array[0] = label0;
        array[1] = label1;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Labels2 rhs = (Labels2) o;
        if (!label0.equals(rhs.label0)) return false;
        return label1.equals(rhs.label1);
    }

    @Override
    public int hashCode() {
        int result = label0.hashCode();
        result = 31 * result + label1.hashCode();
        return result;
    }

}
