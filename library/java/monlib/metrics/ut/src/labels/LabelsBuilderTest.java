package ru.yandex.monlib.metrics.labels;

import org.junit.Assert;
import org.junit.Test;

import ru.yandex.monlib.metrics.labels.LabelsBuilder.SortState;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;


/**
 * @author Sergey Polovko
 */
public class LabelsBuilderTest {

    @Test
    public void constructor() throws Exception {
        LabelsBuilder sorted = new LabelsBuilder(SortState.SORTED,
            label("a", "1"),
            label("b", "2"),
            label("c", "3"));
        assertEquals(3, sorted.size());
        assertTrue(isSorted(sorted));

        LabelsBuilder notSorted = new LabelsBuilder(SortState.MAYBE_NOT_SORTED,
            label("b", "2"),
            label("c", "3"),
            label("a", "1"));
        assertEquals(3, notSorted.size());
        assertTrue(isSorted(notSorted));

        try {
            // duplicated keys
            new LabelsBuilder(SortState.MAYBE_NOT_SORTED,
                label("b", "1"),
                label("b", "2"),
                label("b", "3"));
            fail("exception not thrown");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalStateException);
        }
    }

    @Test
    public void str() {
        LabelsBuilder builder = new LabelsBuilder(Labels.MAX_LABELS_COUNT);
        builder.add("one", "1");
        builder.add("two", "2");
        builder.add("three", "3");
        Assert.assertEquals("{one='1', three='3', two='2'}", builder.toString());
    }

    private static Label label(String key, String value) {
        return Labels.allocator.alloc(key, value);
    }

    private static boolean isSorted(LabelsBuilder b) {
        for (int i = 1; i < b.size(); i++) {
            Label prev = b.at(i - 1);
            Label curr = b.at(i);
            if (prev.compareKeys(curr) > 0) {
                return false;
            }
        }
        return true;
    }
}
