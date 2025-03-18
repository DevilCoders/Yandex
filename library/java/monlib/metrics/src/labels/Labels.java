package ru.yandex.monlib.metrics.labels;

import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import java.util.stream.Stream;

import javax.annotation.Nullable;
import javax.annotation.ParametersAreNonnullByDefault;
import javax.annotation.concurrent.Immutable;


/**
 * @author Sergey Polovko
 */
@Immutable
@ParametersAreNonnullByDefault
public abstract class Labels extends LabelsStaticPart {

    // - size & index access -

    public abstract int size();
    public abstract boolean isEmpty();
    public abstract Label at(int index);

    // - iteration -

    public abstract void forEach(Consumer<Label> labelConsumer);
    public abstract Stream<Label> stream();

    // - find -

    public abstract int indexByKey(String key);
    public abstract int indexBySameKey(Label label);

    public boolean hasKey(String key) {
        return indexByKey(key) != -1;
    }

    public boolean hasSameKey(Label label) {
        return indexBySameKey(label) != -1;
    }

    @Nullable
    public abstract Label findByKey(String key);
    @Nullable
    public abstract Label findBySameKey(Label label);

    // - mutations -

    public Labels add(String key, String value) {
        return toBuilder().add(key, value).build();
    }

    public Labels add(Label label) {
        return toBuilder().add(label).build();
    }

    public Labels addAll(Labels rhs) {
        if (isEmpty()) {
            return rhs;
        } else if (rhs.isEmpty()) {
            return this;
        }
        return toBuilder().addAll(rhs).build();
    }

    public Labels removeByKey(String key) {
        final int index = indexByKey(key);
        if (index >= 0) {
            return removeByIndex(index);
        }
        return this;
    }

    public Labels removeBySameKey(Label label) {
        final int index = indexBySameKey(label);
        if (index >= 0) {
            return removeByIndex(index);
        }
        return this;
    }

    public Labels removeByIndex(int index) {
        if (index < 0 || index >= size()) {
            final String msg = String.format("index: %d, is out of [0,%d)", index, size());
            throw new IndexOutOfBoundsException(msg);
        }
        return toBuilder().remove(index).build();
    }

    // - transformations -

    public abstract LabelsBuilder toBuilder();
    public abstract Map<String, String> toMap();
    public abstract List<Label> toList();

    public Label[] toArray() {
        Label[] labels = new Label[size()];
        copyInto(labels);
        return labels;
    }

    // given array must have enough space
    // elements in range [size(), array.length) will not be touched
    public abstract void copyInto(Label[] array);

    // - helper functions -

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append('{');
        forEach(label -> {
            label.toString(sb);
            sb.append(',');
            sb.append(' ');
        });
        if (sb.length() > 2) {
            sb.setLength(sb.length() - 2);
        }
        sb.append('}');
        return sb.toString();
    }

    protected void checkIndex(int index) {
        if (index < 0 || index >= size()) {
            final String msg = String.format("index: %d, is out of [0, %d)", index, size());
            throw new IndexOutOfBoundsException(msg);
        }
    }
}
