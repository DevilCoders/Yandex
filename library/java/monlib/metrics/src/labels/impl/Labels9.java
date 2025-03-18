
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
public final class Labels9 extends Labels {

    private final Label label0;
    private final Label label1;
    private final Label label2;
    private final Label label3;
    private final Label label4;
    private final Label label5;
    private final Label label6;
    private final Label label7;
    private final Label label8;

    public Labels9(Label label0, Label label1, Label label2, Label label3, Label label4, Label label5, Label label6, Label label7, Label label8) {
        this.label0 = label0;
        this.label1 = label1;
        this.label2 = label2;
        this.label3 = label3;
        this.label4 = label4;
        this.label5 = label5;
        this.label6 = label6;
        this.label7 = label7;
        this.label8 = label8;
    }

    public Labels9(LabelsBuilder builder) {
        this.label0 = builder.at(0);
        this.label1 = builder.at(1);
        this.label2 = builder.at(2);
        this.label3 = builder.at(3);
        this.label4 = builder.at(4);
        this.label5 = builder.at(5);
        this.label6 = builder.at(6);
        this.label7 = builder.at(7);
        this.label8 = builder.at(8);
    }

    @Override
    public int size() {
        return 9;
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
            case 4: return label4;
            case 5: return label5;
            case 6: return label6;
            case 7: return label7;
            case 8: return label8;
        }
        return null; // never get here
    }

    @Override
    public void forEach(Consumer<Label> c) {
        c.accept(label0);
        c.accept(label1);
        c.accept(label2);
        c.accept(label3);
        c.accept(label4);
        c.accept(label5);
        c.accept(label6);
        c.accept(label7);
        c.accept(label8);
    }

    @Override
    public Stream<Label> stream() {
        return Stream.of(label0, label1, label2, label3, label4, label5, label6, label7, label8);
    }

    @Override
    public int indexByKey(String key) {
        LabelsValidator.checkKeyValid(key);
        if (label0.hasKey(key)) return 0;
        if (label1.hasKey(key)) return 1;
        if (label2.hasKey(key)) return 2;
        if (label3.hasKey(key)) return 3;
        if (label4.hasKey(key)) return 4;
        if (label5.hasKey(key)) return 5;
        if (label6.hasKey(key)) return 6;
        if (label7.hasKey(key)) return 7;
        if (label8.hasKey(key)) return 8;
        return -1;
    }

    @Override
    public int indexBySameKey(Label label) {
        if (label0.hasSameKey(label)) return 0;
        if (label1.hasSameKey(label)) return 1;
        if (label2.hasSameKey(label)) return 2;
        if (label3.hasSameKey(label)) return 3;
        if (label4.hasSameKey(label)) return 4;
        if (label5.hasSameKey(label)) return 5;
        if (label6.hasSameKey(label)) return 6;
        if (label7.hasSameKey(label)) return 7;
        if (label8.hasSameKey(label)) return 8;
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
        if (label4.hasKey(key)) return label4;
        if (label5.hasKey(key)) return label5;
        if (label6.hasKey(key)) return label6;
        if (label7.hasKey(key)) return label7;
        if (label8.hasKey(key)) return label8;
        return null;
    }

    @Override
    @Nullable
    public Label findBySameKey(Label label) {
        if (label0.hasSameKey(label)) return label0;
        if (label1.hasSameKey(label)) return label1;
        if (label2.hasSameKey(label)) return label2;
        if (label3.hasSameKey(label)) return label3;
        if (label4.hasSameKey(label)) return label4;
        if (label5.hasSameKey(label)) return label5;
        if (label6.hasSameKey(label)) return label6;
        if (label7.hasSameKey(label)) return label7;
        if (label8.hasSameKey(label)) return label8;
        return null;
    }

    @Override
    public LabelsBuilder toBuilder() {
        return new LabelsBuilder(SortState.SORTED, label0, label1, label2, label3, label4, label5, label6, label7, label8);
    }

    @Override
    public Map<String, String> toMap() {
        Map<String, String> map = new HashMap<>(size());
        map.put(label0.getKey(), label0.getValue());
        map.put(label1.getKey(), label1.getValue());
        map.put(label2.getKey(), label2.getValue());
        map.put(label3.getKey(), label3.getValue());
        map.put(label4.getKey(), label4.getValue());
        map.put(label5.getKey(), label5.getValue());
        map.put(label6.getKey(), label6.getValue());
        map.put(label7.getKey(), label7.getValue());
        map.put(label8.getKey(), label8.getValue());
        return map;
    }

    @Override
    public List<Label> toList() {
        return Arrays.asList(label0, label1, label2, label3, label4, label5, label6, label7, label8);
    }

    @Override
    public void copyInto(Label[] array) {
        array[0] = label0;
        array[1] = label1;
        array[2] = label2;
        array[3] = label3;
        array[4] = label4;
        array[5] = label5;
        array[6] = label6;
        array[7] = label7;
        array[8] = label8;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Labels9 rhs = (Labels9) o;
        if (!label0.equals(rhs.label0)) return false;
        if (!label1.equals(rhs.label1)) return false;
        if (!label2.equals(rhs.label2)) return false;
        if (!label3.equals(rhs.label3)) return false;
        if (!label4.equals(rhs.label4)) return false;
        if (!label5.equals(rhs.label5)) return false;
        if (!label6.equals(rhs.label6)) return false;
        if (!label7.equals(rhs.label7)) return false;
        return label8.equals(rhs.label8);
    }

    @Override
    public int hashCode() {
        int result = label0.hashCode();
        result = 31 * result + label1.hashCode();
        result = 31 * result + label2.hashCode();
        result = 31 * result + label3.hashCode();
        result = 31 * result + label4.hashCode();
        result = 31 * result + label5.hashCode();
        result = 31 * result + label6.hashCode();
        result = 31 * result + label7.hashCode();
        result = 31 * result + label8.hashCode();
        return result;
    }

}
