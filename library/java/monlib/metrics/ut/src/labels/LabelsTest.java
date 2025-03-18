package ru.yandex.monlib.metrics.labels;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.junit.Assert;
import org.junit.Test;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;


/**
 * @author Sergey Polovko
 */
public class LabelsTest {

//    static {
//        System.setProperty("ru.yandex.solomon.labelAllocator", "bytes");
//    }

    @Test
    public void size() throws Exception {
        assertEquals(0, Labels.empty().size());
        assertEquals(0, Labels.of().size());
        assertEquals(1, Labels.of("some", "labels").size());
        assertEquals(2, Labels.of("one", "more", "labels", "example").size());
        assertEquals(0, Labels.builder(1).build().size());
        assertEquals(1, Labels.builder().add("some", "label").build().size());
    }

    @Test
    public void isEmpty() throws Exception {
        assertTrue(Labels.empty().isEmpty());
        assertTrue(Labels.of().isEmpty());
        assertFalse(Labels.of("some", "label").isEmpty());
        assertTrue(Labels.builder(1).build().isEmpty());
        assertFalse(Labels.builder().add("some", "label").build().isEmpty());
    }

    @Test
    public void at() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        // labels are always sorted by key
        assertEquals(Labels.allocator.alloc("one", "1"), labels.at(0));
        assertEquals(Labels.allocator.alloc("three", "3"), labels.at(1));
        assertEquals(Labels.allocator.alloc("two", "2"), labels.at(2));

        try {
            labels.at(-1);
            Assert.fail("exception is not thrown");
        } catch (Exception e) {
            assertTrue(e instanceof IndexOutOfBoundsException);
        }
        try {
            labels.at(3);
            Assert.fail("exception is not thrown");
        } catch (Exception e) {
            assertTrue(e instanceof IndexOutOfBoundsException);
        }
    }

    @Test
    public void forEach() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        List<Label> labelsList = new ArrayList<>(labels.size());
        labels.forEach(labelsList::add);

        assertEquals(labels.size(), labelsList.size());
        for (int i = 0; i < labels.size(); i++) {
            assertEquals(labels.at(i), labelsList.get(i));
        }
    }

    @Test
    public void findIndexByKey() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        // labels are always sorted by key
        assertEquals(0, labels.indexByKey("one"));
        assertEquals(1, labels.indexByKey("three"));
        assertEquals(2, labels.indexByKey("two"));

        assertEquals(-1, labels.indexByKey("four"));
    }

    @Test
    public void indexBySameKey() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        // labels are always sorted by key
        assertEquals(0, labels.indexBySameKey(Labels.allocator.alloc("one", "another1")));
        assertEquals(1, labels.indexBySameKey(Labels.allocator.alloc("three", "another3")));
        assertEquals(2, labels.indexBySameKey(Labels.allocator.alloc("two", "another2")));

        assertEquals(-1, labels.indexBySameKey(Labels.allocator.alloc("four", "4")));
    }

    @Test
    public void hasKey() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        for (String key : Arrays.asList("one", "two", "three")) {
            assertTrue(key + " not found", labels.hasKey(key));
        }
        assertFalse(labels.hasKey("four"));
    }

    @Test
    public void hasSameKey() throws Exception {
        LabelAllocator a = Labels.allocator;
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        for (String key : Arrays.asList("one", "two", "three")) {
            Label l1 = a.alloc(key, "another");
            assertTrue(key + " not found", labels.hasSameKey(l1));
            Label l2 = a.alloc(key, "1");
            assertTrue(key + " not found", labels.hasSameKey(l2));
        }
        assertFalse(labels.hasSameKey(a.alloc("four", "4")));
    }

    @Test
    public void findByKey() throws Exception {
        LabelAllocator a = Labels.allocator;
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        assertEquals(a.alloc("one", "1"), labels.findByKey("one"));
        assertEquals(a.alloc("two", "2"), labels.findByKey("two"));
        assertEquals(a.alloc("three", "3"), labels.findByKey("three"));
        assertNull(labels.findByKey("four"));
    }

    @Test
    public void findBySameKey() throws Exception {
        LabelAllocator a = Labels.allocator;
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        assertEquals(a.alloc("one", "1"), labels.findBySameKey(a.alloc("one", "another1")));
        assertEquals(a.alloc("two", "2"), labels.findBySameKey(a.alloc("two", "another2")));
        assertEquals(a.alloc("three", "3"), labels.findBySameKey(a.alloc("three", "another3")));
        assertNull(labels.findByKey("four"));
    }

    @Test
    public void add1() throws Exception {
        Labels labels = Labels.empty();

        Labels one = labels.add("one", "1");
        assertEquals(Labels.of("one", "1"), one);

        Labels two = one.add("two", "2");
        assertEquals(Labels.of("one", "1"), one);
        assertEquals(Labels.of("one", "1", "two", "2"), two);

        Labels three = two.add("three", "3");
        assertEquals(Labels.of("one", "1"), one);
        assertEquals(Labels.of("one", "1", "two", "2"), two);
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), three);

        // add replaces existed label
        assertEquals(Labels.of("one", "111", "two", "2"), two.add("one", "111"));
    }

    @Test
    public void add2() throws Exception {
        LabelAllocator a = Labels.allocator;
        Labels labels = Labels.empty();

        Labels one = labels.add(a.alloc("one", "1"));
        assertEquals(Labels.of("one", "1"), one);

        Labels two = one.add(a.alloc("two", "2"));
        assertEquals(Labels.of("one", "1"), one);
        assertEquals(Labels.of("one", "1", "two", "2"), two);

        Labels three = two.add(a.alloc("three", "3"));
        assertEquals(Labels.of("one", "1"), one);
        assertEquals(Labels.of("one", "1", "two", "2"), two);
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), three);

        // add replaces existed label
        assertEquals(Labels.of("one", "111", "two", "2"), two.add(a.alloc("one", "111")));
    }

    @Test
    public void removeByKey() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        Labels one = labels.removeByKey("one");
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), labels);
        assertEquals(Labels.of("two", "2", "three", "3"), one);

        Labels two = one.removeByKey("two");
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), labels);
        assertEquals(Labels.of("two", "2", "three", "3"), one);
        assertEquals(Labels.of("three", "3"), two);

        Labels three = two.removeByKey("three");
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), labels);
        assertEquals(Labels.of("two", "2", "three", "3"), one);
        assertEquals(Labels.of("three", "3"), two);
        assertEquals(Labels.empty(), three);

        // if key is not found object must not be changed
        Labels four = labels.removeByKey("four");
        assertSame(labels, four);
    }

    @Test
    public void removeBySameKey() throws Exception {
        LabelAllocator a = Labels.allocator;
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        Labels one = labels.removeBySameKey(a.alloc("one", "another1"));
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), labels);
        assertEquals(Labels.of("two", "2", "three", "3"), one);

        Labels two = one.removeBySameKey(a.alloc("two", "another2"));
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), labels);
        assertEquals(Labels.of("two", "2", "three", "3"), one);
        assertEquals(Labels.of("three", "3"), two);

        Labels three = two.removeBySameKey(a.alloc("three", "another3"));
        assertEquals(Labels.of("one", "1", "two", "2", "three", "3"), labels);
        assertEquals(Labels.of("two", "2", "three", "3"), one);
        assertEquals(Labels.of("three", "3"), two);
        assertEquals(Labels.empty(), three);

        // if key is not found object must not be changed
        Labels four = labels.removeBySameKey(a.alloc("four", "another4"));
        assertSame(labels, four);
    }

    @Test
    public void removeByIndex() throws Exception {
        // labels are sorted by key
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        assertEquals(Labels.of("two", "2", "three", "3"), labels.removeByIndex(0));
        assertEquals(Labels.of("one", "1", "two", "2"), labels.removeByIndex(1));
        assertEquals(Labels.of("one", "1", "three", "3"), labels.removeByIndex(2));

        Labels empty = labels.removeByIndex(0).removeByIndex(0).removeByIndex(0);
        assertEquals(Labels.empty(), empty);

        try {
            labels.removeByIndex(-1);
            Assert.fail("exception is not thrown");
        } catch (Exception e) {
            assertTrue(e instanceof IndexOutOfBoundsException);
        }
        try {
            labels.removeByIndex(3);
            Assert.fail("exception is not thrown");
        } catch (Exception e) {
            assertTrue(e instanceof IndexOutOfBoundsException);
        }
    }

    @Test
    public void toBuilder() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        LabelsBuilder builder = labels.toBuilder();
        assertEquals(labels, builder.build());
    }

    @Test
    public void toMap() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        Map<String, String> map = new HashMap<>(3);
        map.put("one", "1");
        map.put("two", "2");
        map.put("three", "3");
        assertEquals(map, labels.toMap());
    }

    @Test
    public void toList() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        LabelAllocator a = Labels.allocator;
        List<Label> list = Arrays.asList(
            a.alloc("one", "1"),
            a.alloc("two", "2"),
            a.alloc("three", "3"));
        list.sort(Label::compareKeys);
        assertEquals(list, labels.toList());
    }

    @Test
    public void toArray() throws Exception {
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");
        LabelAllocator a = Labels.allocator;
        Label[] array = {
            a.alloc("one", "1"),
            a.alloc("two", "2"),
            a.alloc("three", "3")
        };
        Arrays.sort(array, Label::compareKeys);
        assertArrayEquals(array, labels.toArray());
    }

    @Test
    public void copyInto() throws Exception {
        LabelAllocator a = Labels.allocator;
        Labels labels = Labels.of("one", "1", "two", "2", "three", "3");

        try {
            labels.copyInto(new Label[1]);
            Assert.fail("must fail");
        } catch (IndexOutOfBoundsException expected) {
            expected.printStackTrace();
        }

        Label[] array = new Label[labels.size() + 1];
        array[labels.size()] = a.alloc("another", "label"); // will not be touched
        labels.copyInto(array);

        Assert.assertEquals(a.alloc("one", "1"), array[0]);
        Assert.assertEquals(a.alloc("three", "3"), array[1]);
        Assert.assertEquals(a.alloc("two", "2"), array[2]);
        Assert.assertEquals(a.alloc("another", "label"), array[3]);
    }

    @Test
    public void testToString() throws Exception {
        assertEquals("{}", Labels.empty().toString());
        assertEquals("{one='1'}", Labels.of("one", "1").toString());
        assertEquals("{one='1', two='2'}", Labels.of("one", "1", "two", "2").toString());
    }
}
