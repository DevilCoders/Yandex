
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
public final class Labels4 extends Labels {

    private final Label label0;
    private final Label label1;
    private final Label label2;
    private final Label label3;

    public Labels4(Label label0, Label label1, Label label2, Label label3) {
        this.label0 = label0;
        this.label1 = label1;
        this.label2 = label2;
        this.label3 = label3;
    }

    public Labels4(LabelsBuilder builder) {
        this.label0 = builder.at(0);
        this.label1 = builder.at(1);
        this.label2 = builder.at(2);
        this.label3 = builder.at(3);
    }

    @Override
    public int size() {
        return 4;
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
            case 2: return label2;
            case 3: return label3;
        }
        return null; // never get here
    }

    @Override
    public void forEach(Consumer<Label> c) {
        c.accept(label0);
        c.accept(label1);
        c.accept(label2);
        c.accept(label3);
    }

    @Override
    public Stream<Label> stream() {
        return Stream.of(label0, label1, label2, label3);
    }

    @Override
    public int indexByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        if (label0.hasKey(key)) return 0;
        if (label1.hasKey(key)) return 1;
        if (label2.hasKey(key)) return 2;
        if (label3.hasKey(key)) return 3;
        return -1;
    }

    @Override
    public int indexBySameKey(Label label) {
        if (label0.hasSameKey(label)) return 0;
        if (label1.hasSameKey(label)) return 1;
        if (label2.hasSameKey(label)) return 2;
        if (label3.hasSameKey(label)) return 3;
        return -1;
    }

    @Override
    @Nullable
    public Label findByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        if (label0.hasKey(key)) return label0;
        if (label1.hasKey(key)) return label1;
        if (label2.hasKey(key)) return label2;
        if (label3.hasKey(key)) return label3;
        return null;
    }

    @Override
    @Nullable
    public Label findBySameKey(Label label) {
        if (label0.hasSameKey(label)) return label0;
        if (label1.hasSameKey(label)) return label1;
        if (label2.hasSameKey(label)) return label2;
        if (label3.hasSameKey(label)) return label3;
        return null;
    }

    @Override
    public LabelsBuilder toBuilder() {
        return new LabelsBuilder(SortState.SORTED, label0, label1, label2, label3);
    }

    @Override
    public Map<String, String> toMap() {
        Map<String, String> map = new HashMap<>(size());
        map.put(label0.getKey(), label0.getValue());
        map.put(label1.getKey(), label1.getValue());
        map.put(label2.getKey(), label2.getValue());
        map.put(label3.getKey(), label3.getValue());
        return map;
    }

    @Override
    public List<Label> toList() {
        return Arrays.asList(label0, label1, label2, label3);
    }

    @Override
    public void copyInto(Label[] array) {
        array[0] = label0;
        array[1] = label1;
        array[2] = label2;
        array[3] = label3;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Labels4 rhs = (Labels4) o;
        if (!label0.equals(rhs.label0)) return false;
        if (!label1.equals(rhs.label1)) return false;
        if (!label2.equals(rhs.label2)) return false;
        return label3.equals(rhs.label3);
    }

    @Override
    public int hashCode() {
        int result = label0.hashCode();
        result = 31 * result + label1.hashCode();
        result = 31 * result + label2.hashCode();
        result = 31 * result + label3.hashCode();
        return result;
    }

}
