package ru.yandex.monlib.metrics.encode.spack;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.junit.Test;

import ru.yandex.monlib.metrics.encode.spack.StringPoolBuilder.PooledString;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;


/**
 * @author Sergey Polovko
 */
public class StringPoolBuilderTest {

    @Test
    public void putIfAbsent() throws Exception {
        StringPoolBuilder builder = new StringPoolBuilder();
        PooledString one = builder.putIfAbsent("one");
        assertNotNull(one);

        PooledString two1 = builder.putIfAbsent("two");
        PooledString two2 = builder.putIfAbsent("two");
        assertSame(two1, two2);

        assertEquals("one", one.value);
        assertEquals(1, one.frequency);
        assertEquals(-1, one.index);

        assertEquals("two", two1.value);
        assertEquals(2, two1.frequency);
        assertEquals(-1, two1.index);
    }

    @Test
    public void sortByFrequencies() throws Exception {
        StringPoolBuilder builder = new StringPoolBuilder();
        PooledString one = builder.putIfAbsent("one");

        PooledString two = builder.putIfAbsent("two");
        builder.putIfAbsent("two");

        PooledString three = builder.putIfAbsent("three");
        builder.putIfAbsent("three");
        builder.putIfAbsent("three");

        assertEquals(-1, one.index);
        assertEquals(-1, two.index);
        assertEquals(-1, three.index);

        builder.sortByFrequencies();

        List<String> strings = new ArrayList<>(builder.size());
        builder.forEachString(strings::add);

        assertEquals(Arrays.asList("three", "two", "one"), strings);

        assertEquals(2, one.index);
        assertEquals(1, two.index);
        assertEquals(0, three.index);
    }

    @Test
    public void getBytesSize() throws Exception {
        StringPoolBuilder builder = new StringPoolBuilder();

        int expectedLen = 0;
        for (String s : Arrays.asList("a", "bbb", "ccc")) {
            expectedLen += s.length();

            builder.putIfAbsent(s);
            assertEquals(expectedLen, builder.getBytesSize());

            builder.putIfAbsent(s);
            assertEquals(expectedLen, builder.getBytesSize());
        }
    }
}
