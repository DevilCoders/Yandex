package ru.yandex.monlib.metrics.labels;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import org.openjdk.jol.info.GraphLayout;

import ru.yandex.monlib.metrics.labels.bytes.BytesLabelAllocator;
import ru.yandex.monlib.metrics.labels.string.StringLabelAllocator;


/**
 * @author Sergey Polovko
 */
@RunWith(Parameterized.class)
public class LabelsLayoutTest {

    @Parameters(name = "keyLen={0}, valueLen={1}")
    public static Collection<Integer[]> parameters() {
        return Arrays.asList(new Integer[][] {
            // keyLen, valueLen
            {       1,        1 }, // minimal
            {      12,       40 }, // typical
            {      32,      200 }, // maximal
        });
    }

    private final int keyLen;
    private final int valueLen;

    public LabelsLayoutTest(int keyLen, int valueLen) {
        this.keyLen = keyLen;
        this.valueLen = valueLen;
    }

    @Test
    public void bytesVsString() throws Exception {
        LabelAllocator[] allocators = {
            StringLabelAllocator.SELF,
            BytesLabelAllocator.SELF,
        };

        StringBuilder sb = new StringBuilder()
            .append(String.format(" size |   class  | %22s | %22s\n",
                allocators[0].getClass().getSimpleName(),
                allocators[1].getClass().getSimpleName()))
            .append("------+----------+------------------------+-------------------------\n");

        for (int size = 0; size <= Labels.MAX_LABELS_COUNT; size++) {
            Labels labels1 = labels(allocators[0], size);
            Labels labels2 = labels(allocators[1], size);

            GraphLayout labels1Layout = GraphLayout.parseInstance(labels1);
            GraphLayout labels2Layout = GraphLayout.parseInstance(labels2);

            sb.append(String.format("%5d | %8s | %22d | %22d \n",
                size,
                labels1.getClass().getSimpleName(),
                labels1Layout.totalSize(),
                labels2Layout.totalSize()));

//            if (size == 16) {
//                System.out.println(labels1Layout.toFootprint());
//                System.out.println(labels2Layout.toFootprint());
//            }
        }
        System.out.println(sb.toString());
    }

    @Test
    public void labelsVsArrays() throws Exception {
        LabelAllocator allocator = BytesLabelAllocator.SELF;
        StringBuilder sb = new StringBuilder()
            .append(" size |   class  | LabelsXX |  LabelsArray | ArrayList<Label> \n")
            .append("------+----------+----------+--------------+------------------\n");

        for (int size = 0; size <= Labels.MAX_LABELS_COUNT; size++) {
            Labels labels = labels(allocator, size);
            LabelsArray labelsArray = labelsArray(allocator, size);
            List<Label> arrayList = arrayList(allocator, size);

            GraphLayout labelsLayout = GraphLayout.parseInstance(labels);
            GraphLayout labelsArrayLayout = GraphLayout.parseInstance(labelsArray);
            GraphLayout arrayListLayout = GraphLayout.parseInstance(arrayList);

            sb.append(String.format("%5d | %8s | %8d | %12d | %15d \n",
                size,
                labels.getClass().getSimpleName(),
                labelsLayout.totalSize(),
                labelsArrayLayout.totalSize(),
                arrayListLayout.totalSize()));

//            if (size == 16) {
//                System.out.println(labelsLayout.toFootprint());
//                System.out.println(labelsArrayLayout.toFootprint());
//                System.out.println(arrayListLayout.toFootprint());
//            }
        }

        System.out.println(sb.toString());
    }

    private Labels labels(LabelAllocator allocator, int size) {
        LabelsBuilder lb = new LabelsBuilder(size);
        StringBuilder sb = new StringBuilder(Math.max(keyLen, valueLen));
        for (int i = 0; i < size; i++) {
            sb.setLength(0);
            for (int j = 0; j < keyLen; j++) {
                sb.append((char) ('a' + i));
            }
            String key = sb.toString();

            sb.setLength(0);
            for (int j = 0; j < valueLen; j++) {
                sb.append((char) ('b' + i));
            }
            String value = sb.toString();

            lb.add(allocator.alloc(key, value));
        }
        return lb.build();
    }

    private LabelsArray labelsArray(LabelAllocator allocator, int size) {
        return new LabelsArray(arrayImpl(allocator, size));
    }

    private List<Label> arrayList(LabelAllocator allocator, int size) {
        return new ArrayList<>(Arrays.asList(arrayImpl(allocator, size)));
    }

    private Label[] arrayImpl(LabelAllocator allocator, int size) {
        Label[] labels = new Label[size];
        StringBuilder sb = new StringBuilder(Math.max(keyLen, valueLen));
        for (int i = 0; i < size; i++) {
            sb.setLength(0);
            for (int j = 0; j < keyLen; j++) {
                sb.append((char) ('a' + i));
            }
            String key = sb.toString();

            sb.setLength(0);
            for (int j = 0; j < valueLen; j++) {
                sb.append((char) ('b' + i));
            }
            String value = sb.toString();

            labels[i] = allocator.alloc(key, value);
        }
        return labels;
    }

    /**
     * LABELS ARRAY
     */
    private static final class LabelsArray {
        private final Label[] labels;

        LabelsArray(Label[] labels) {
            this.labels = labels;
        }
    }
}
