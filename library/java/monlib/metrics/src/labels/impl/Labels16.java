
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
public final class Labels16 extends Labels {

    private final Label label0;
    private final Label label1;
    private final Label label2;
    private final Label label3;
    private final Label label4;
    private final Label label5;
    private final Label label6;
    private final Label label7;
    private final Label label8;
    private final Label label9;
    private final Label label10;
    private final Label label11;
    private final Label label12;
    private final Label label13;
    private final Label label14;
    private final Label label15;

    public Labels16(Label label0, Label label1, Label label2, Label label3, Label label4, Label label5, Label label6, Label label7, Label label8, Label label9, Label label10, Label label11, Label label12, Label label13, Label label14, Label label15) {
        this.label0 = label0;
        this.label1 = label1;
        this.label2 = label2;
        this.label3 = label3;
        this.label4 = label4;
        this.label5 = label5;
        this.label6 = label6;
        this.label7 = label7;
        this.label8 = label8;
        this.label9 = label9;
        this.label10 = label10;
        this.label11 = label11;
        this.label12 = label12;
        this.label13 = label13;
        this.label14 = label14;
        this.label15 = label15;
    }

    public Labels16(LabelsBuilder builder) {
        this.label0 = builder.at(0);
        this.label1 = builder.at(1);
        this.label2 = builder.at(2);
        this.label3 = builder.at(3);
        this.label4 = builder.at(4);
        this.label5 = builder.at(5);
        this.label6 = builder.at(6);
        this.label7 = builder.at(7);
        this.label8 = builder.at(8);
        this.label9 = builder.at(9);
        this.label10 = builder.at(10);
        this.label11 = builder.at(11);
        this.label12 = builder.at(12);
        this.label13 = builder.at(13);
        this.label14 = builder.at(14);
        this.label15 = builder.at(15);
    }

    @Override
    public int size() {
        return 16;
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
            case 9: return label9;
            case 10: return label10;
            case 11: return label11;
            case 12: return label12;
            case 13: return label13;
            case 14: return label14;
            case 15: return label15;
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
        c.accept(label9);
        c.accept(label10);
        c.accept(label11);
        c.accept(label12);
        c.accept(label13);
        c.accept(label14);
        c.accept(label15);
    }

    @Override
    public Stream<Label> stream() {
        return Stream.of(label0, label1, label2, label3, label4, label5, label6, label7, label8, label9, label10, label11, label12, label13, label14, label15);
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
        if (label9.hasKey(key)) return 9;
        if (label10.hasKey(key)) return 10;
        if (label11.hasKey(key)) return 11;
        if (label12.hasKey(key)) return 12;
        if (label13.hasKey(key)) return 13;
        if (label14.hasKey(key)) return 14;
        if (label15.hasKey(key)) return 15;
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
        if (label9.hasSameKey(label)) return 9;
        if (label10.hasSameKey(label)) return 10;
        if (label11.hasSameKey(label)) return 11;
        if (label12.hasSameKey(label)) return 12;
        if (label13.hasSameKey(label)) return 13;
        if (label14.hasSameKey(label)) return 14;
        if (label15.hasSameKey(label)) return 15;
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
        if (label9.hasKey(key)) return label9;
        if (label10.hasKey(key)) return label10;
        if (label11.hasKey(key)) return label11;
        if (label12.hasKey(key)) return label12;
        if (label13.hasKey(key)) return label13;
        if (label14.hasKey(key)) return label14;
        if (label15.hasKey(key)) return label15;
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
        if (label9.hasSameKey(label)) return label9;
        if (label10.hasSameKey(label)) return label10;
        if (label11.hasSameKey(label)) return label11;
        if (label12.hasSameKey(label)) return label12;
        if (label13.hasSameKey(label)) return label13;
        if (label14.hasSameKey(label)) return label14;
        if (label15.hasSameKey(label)) return label15;
        return null;
    }

    @Override
    public LabelsBuilder toBuilder() {
        return new LabelsBuilder(SortState.SORTED, label0, label1, label2, label3, label4, label5, label6, label7, label8, label9, label10, label11, label12, label13, label14, label15);
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
        map.put(label9.getKey(), label9.getValue());
        map.put(label10.getKey(), label10.getValue());
        map.put(label11.getKey(), label11.getValue());
        map.put(label12.getKey(), label12.getValue());
        map.put(label13.getKey(), label13.getValue());
        map.put(label14.getKey(), label14.getValue());
        map.put(label15.getKey(), label15.getValue());
        return map;
    }

    @Override
    public List<Label> toList() {
        return Arrays.asList(label0, label1, label2, label3, label4, label5, label6, label7, label8, label9, label10, label11, label12, label13, label14, label15);
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
        array[9] = label9;
        array[10] = label10;
        array[11] = label11;
        array[12] = label12;
        array[13] = label13;
        array[14] = label14;
        array[15] = label15;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Labels16 rhs = (Labels16) o;
        if (!label0.equals(rhs.label0)) return false;
        if (!label1.equals(rhs.label1)) return false;
        if (!label2.equals(rhs.label2)) return false;
        if (!label3.equals(rhs.label3)) return false;
        if (!label4.equals(rhs.label4)) return false;
        if (!label5.equals(rhs.label5)) return false;
        if (!label6.equals(rhs.label6)) return false;
        if (!label7.equals(rhs.label7)) return false;
        if (!label8.equals(rhs.label8)) return false;
        if (!label9.equals(rhs.label9)) return false;
        if (!label10.equals(rhs.label10)) return false;
        if (!label11.equals(rhs.label11)) return false;
        if (!label12.equals(rhs.label12)) return false;
        if (!label13.equals(rhs.label13)) return false;
        if (!label14.equals(rhs.label14)) return false;
        return label15.equals(rhs.label15);
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
        result = 31 * result + label9.hashCode();
        result = 31 * result + label10.hashCode();
        result = 31 * result + label11.hashCode();
        result = 31 * result + label12.hashCode();
        result = 31 * result + label13.hashCode();
        result = 31 * result + label14.hashCode();
        result = 31 * result + label15.hashCode();
        return result;
    }

}
