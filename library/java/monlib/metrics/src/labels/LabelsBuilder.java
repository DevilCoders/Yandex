package ru.yandex.monlib.metrics.labels;

import java.util.Arrays;
import java.util.Map;
import java.util.function.Consumer;

import javax.annotation.ParametersAreNonnullByDefault;

import ru.yandex.monlib.metrics.labels.impl.Labels0;
import ru.yandex.monlib.metrics.labels.impl.Labels1;
import ru.yandex.monlib.metrics.labels.impl.Labels10;
import ru.yandex.monlib.metrics.labels.impl.Labels11;
import ru.yandex.monlib.metrics.labels.impl.Labels12;
import ru.yandex.monlib.metrics.labels.impl.Labels13;
import ru.yandex.monlib.metrics.labels.impl.Labels14;
import ru.yandex.monlib.metrics.labels.impl.Labels15;
import ru.yandex.monlib.metrics.labels.impl.Labels16;
import ru.yandex.monlib.metrics.labels.impl.Labels2;
import ru.yandex.monlib.metrics.labels.impl.Labels3;
import ru.yandex.monlib.metrics.labels.impl.Labels4;
import ru.yandex.monlib.metrics.labels.impl.Labels5;
import ru.yandex.monlib.metrics.labels.impl.Labels6;
import ru.yandex.monlib.metrics.labels.impl.Labels7;
import ru.yandex.monlib.metrics.labels.impl.Labels8;
import ru.yandex.monlib.metrics.labels.impl.Labels9;
import ru.yandex.monlib.metrics.labels.validate.LabelsValidator;
import ru.yandex.monlib.metrics.labels.validate.TooManyLabelsException;


/**
 * @author Sergey Polovko
 */
@ParametersAreNonnullByDefault
public final class LabelsBuilder {

    public enum SortState {
        SORTED,
        MAYBE_NOT_SORTED,
    }

    private final LabelAllocator allocator;
    private Label[] labels;
    private int size;

    public LabelsBuilder(SortState sortState, Label... labels) {
        LabelsValidator.checkCountValid(labels.length);
        this.allocator = Labels.allocator;
        this.labels = labels;
        if (sortState == SortState.MAYBE_NOT_SORTED) {
            Arrays.sort(this.labels, (l1, l2) -> {
                final int result = l1.compareKeys(l2);
                if (result == 0) {
                    throw new IllegalStateException("key duplication in labels array: " + l1.getKey());
                }
                return result;
            });
        }
        this.size = this.labels.length;
    }

    public LabelsBuilder(int capacity) {
        this(capacity, Labels.allocator);
    }

    public LabelsBuilder(int capacity, LabelAllocator allocator) {
        this.allocator = allocator;
        this.labels = new Label[Math.min(capacity, Labels.MAX_LABELS_COUNT)];
        this.size = 0;
    }

    public int size() {
        return size;
    }

    public boolean isEmpty() {
        return size == 0;
    }

    public Label at(int index) {
        return labels[index];
    }

    public int indexOf(String key) {
        LabelsValidator.checkKeyValid(key);
        return binarySearchByKey(key);
    }

    public boolean hasKey(String key) {
        LabelsValidator.checkKeyValid(key);
        return binarySearchByKey(key) >= 0;
    }

    public LabelsBuilder add(String key, String value) {
        return add(allocator.alloc(key, value));
    }

    public LabelsBuilder add(Label label) {
        final int pos = binarySearchByLabel(label);
        if (pos >= 0) {
            // found label with same key, so replace it with new one
            labels[pos] = label;
        } else {
            // new label has unique key, so try to add it
            LabelsValidator.checkCountValid(size + 1);

            final int insertionPos = -pos - 1;
            if (size < labels.length - 1) {
                if (insertionPos < size) {
                    System.arraycopy(labels, insertionPos, labels, insertionPos + 1, size - insertionPos);
                }
            } else {
                Label[] newLabels = new Label[labels.length + 1];
                System.arraycopy(labels, 0, newLabels, 0, insertionPos);
                System.arraycopy(labels, insertionPos, newLabels, insertionPos + 1, labels.length - insertionPos);
                labels = newLabels;
            }
            labels[insertionPos] = label;
            size++;
        }
        return this;
    }

    public LabelsBuilder addAll(Map<String, String> labels) {
        for (Map.Entry<String, String> e : labels.entrySet()) {
            add(e.getKey(), e.getValue());
        }
        return this;
    }

    public LabelsBuilder addAll(Labels labels) {
        if (size == 0) {
            // no need to override already existed labels
            if (this.labels.length < labels.size()) {
                this.labels = new Label[labels.size()];
            }
            labels.copyInto(this.labels);
            this.size = labels.size();
        } else {
            // must add each label individually to make sure that we have unique label keys
            for (int i = 0; i < labels.size(); i++) {
                add(labels.at(i));
            }
        }
        return this;
    }

    public LabelsBuilder remove(Label label) {
        removeImpl(binarySearchByLabel(label));
        return this;
    }

    public LabelsBuilder remove(String key) {
        LabelsValidator.checkKeyValid(key);
        removeImpl(binarySearchByKey(key));
        return this;
    }

    public LabelsBuilder remove(int index) {
        removeImpl(index);
        return this;
    }

    private void removeImpl(int pos) {
        if (pos >= 0 && pos < size) {
            final int lastPos = size - 1;
            if (pos < lastPos) {
                System.arraycopy(labels, pos + 1, labels, pos, lastPos - pos);
            }
            labels[lastPos] = null;
            size--;
        }
    }

    public void forEach(Consumer<Label> consumer) {
        for (int i = 0; i < size; i++) {
            consumer.accept(labels[i]);
        }
    }

    public Labels build() {
        switch (size) {
            case 0: return Labels0.SELF;
            case 1: return new Labels1(this);
            case 2: return new Labels2(this);
            case 3: return new Labels3(this);
            case 4: return new Labels4(this);
            case 5: return new Labels5(this);
            case 6: return new Labels6(this);
            case 7: return new Labels7(this);
            case 8: return new Labels8(this);
            case 9: return new Labels9(this);
            case 10: return new Labels10(this);
            case 11: return new Labels11(this);
            case 12: return new Labels12(this);
            case 13: return new Labels13(this);
            case 14: return new Labels14(this);
            case 15: return new Labels15(this);
            case 16: return new Labels16(this);
        }
        throw new TooManyLabelsException("size: " + size);
    }

    public void clear() {
        Arrays.fill(labels, null);
        size = 0;
    }

    private int binarySearchByKey(String key) {
        int low = 0;
        int high = size - 1;

        while (low <= high) {
            int mid = (low + high) >>> 1;
            Label midVal = labels[mid];
            int cmp = midVal.compareKey(key);
            if (cmp < 0)
                low = mid + 1;
            else if (cmp > 0)
                high = mid - 1;
            else
                return mid; // key found
        }
        return -(low + 1);  // key not found.
    }

    private int binarySearchByLabel(Label label) {
        int low = 0;
        int high = size - 1;

        while (low <= high) {
            int mid = (low + high) >>> 1;
            Label midVal = labels[mid];
            int cmp = midVal.compareKeys(label);
            if (cmp < 0) {
                low = mid + 1;
            } else if (cmp > 0) {
                high = mid - 1;
            } else {
                return mid; // key found
            }
        }
        return -(low + 1);  // key not found.
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(128);
        sb.append('{');
        for (int i = 0; i < size; i++) {
            Label label = labels[i];
            label.toString(sb);
            sb.append(',');
            sb.append(' ');
        }
        if (sb.length() > 2) {
            sb.setLength(sb.length() - 2);
        }
        sb.append('}');
        return sb.toString();
    }
}
