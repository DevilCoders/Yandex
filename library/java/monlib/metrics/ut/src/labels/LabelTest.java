package ru.yandex.monlib.metrics.labels;

import org.junit.Test;

import ru.yandex.monlib.metrics.labels.bytes.BytesLabelAllocator;
import ru.yandex.monlib.metrics.labels.string.StringLabelAllocator;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * @author Sergey Polovko
 */
public class LabelTest {

    LabelAllocator[] allocators = {
        StringLabelAllocator.SELF,
        BytesLabelAllocator.SELF,
    };

    @Test
    public void getKey() throws Exception {
        for (LabelAllocator a : allocators) {
            assertEquals("some", a.alloc("some", "label").getKey());
            assertEquals("another", a.alloc("another", "label with spaces").getKey());
            assertEquals("even", a.alloc("even", "label (with) [some,punctuations]").getKey());
        }
    }

    @Test
    public void getValue() throws Exception {
        for (LabelAllocator a : allocators) {
            assertEquals("label", a.alloc("some", "label").getValue());
            assertEquals("label with spaces", a.alloc("another", "label with spaces").getValue());
            assertEquals("label (with) [some,punctuations]", a.alloc("even", "label (with) [some,punctuations]").getValue());
        }
    }

    @Test
    public void getKeyCharAt() throws Exception {
        for (LabelAllocator a : allocators) {
            {
                Label label = a.alloc("some", "label");
                byte[] buf = new byte[label.getKeyLength()];
                for (int i = 0; i < buf.length; i++) {
                    buf[i] = label.getKeyCharAt(i);
                }
                assertArrayEquals(new byte[]{ 's', 'o', 'm', 'e' }, buf);
            }
            {
                Label label = a.alloc("another", "label with spaces");
                byte[] buf = new byte[label.getKeyLength()];
                for (int i = 0; i < buf.length; i++) {
                    buf[i] = label.getKeyCharAt(i);
                }
                assertArrayEquals(new byte[]{ 'a', 'n', 'o', 't', 'h', 'e', 'r' }, buf);
            }
            {
                Label label = a.alloc("even", "label (with) [some,punctuations]");
                byte[] buf = new byte[label.getKeyLength()];
                for (int i = 0; i < buf.length; i++) {
                    buf[i] = label.getKeyCharAt(i);
                }
                assertArrayEquals(new byte[]{ 'e', 'v', 'e', 'n' }, buf);
            }
        }
    }

    @Test
    public void getValueCharAt() throws Exception {
        for (LabelAllocator a : allocators) {
            {
                Label label = a.alloc("some", "label");
                byte[] buf = new byte[label.getValueLength()];
                for (int i = 0; i < buf.length; i++) {
                    buf[i] = label.getValueCharAt(i);
                }
                assertArrayEquals(new byte[]{ 'l', 'a', 'b', 'e', 'l' }, buf);
            }
            {
                Label label = a.alloc("another", "label with spaces");
                byte[] buf = new byte[label.getValueLength()];
                for (int i = 0; i < buf.length; i++) {
                    buf[i] = label.getValueCharAt(i);
                }
                assertArrayEquals(
                    new byte[]{
                        'l', 'a', 'b', 'e', 'l', ' ',
                        'w', 'i', 't', 'h', ' ',
                        's', 'p', 'a', 'c', 'e', 's'
                    }, buf);
            }
            {
                Label label = a.alloc("even", "label (with) [some,punctuations]");
                byte[] buf = new byte[label.getValueLength()];
                for (int i = 0; i < buf.length; i++) {
                    buf[i] = label.getValueCharAt(i);
                }
                assertArrayEquals(
                    new byte[]{
                        'l', 'a', 'b', 'e', 'l', ' ',
                        '(', 'w', 'i', 't', 'h', ')', ' ',
                        '[', 's', 'o', 'm', 'e', ',', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n', 's', ']'
                    },
                    buf);
            }
            assertEquals("label", a.alloc("some", "label").getValue());
            assertEquals("label with spaces", a.alloc("another", "label with spaces").getValue());
            assertEquals("label (with) [some,punctuations]", a.alloc("even", "label (with) [some,punctuations]").getValue());
        }
    }

    @Test
    public void hasKey() throws Exception {
        for (LabelAllocator a : allocators) {
            assertTrue(a.alloc("some", "label").hasKey("some"));
            assertTrue(a.alloc("another", "label with spaces").hasKey("another"));
            assertTrue(a.alloc("even", "label (with) [some,punctuations]").hasKey("even"));
        }
    }

    @Test
    public void hasSameKey() throws Exception {
        for (LabelAllocator a1 : allocators) {
            for (LabelAllocator a2 : allocators) {
                assertTrue(a1.alloc("some", "label").hasSameKey(a2.alloc("some", "other value")));
                assertTrue(a1.alloc("another", "label with spaces").hasSameKey(a2.alloc("another", "another value")));
                assertTrue(a1.alloc("even", "label (with) [some,punctuations]").hasSameKey(a2.alloc("even", "another value")));
            }
        }
    }

    @Test
    public void compareKey() throws Exception {
        for (LabelAllocator a : allocators) {
            Label l = a.alloc("cde", "value");
            assertTrue(l.compareKey("abc") > 0);
            assertTrue(l.compareKey("def") < 0);
            assertTrue(l.compareKey("cde") == 0);
        }
    }

    @Test
    public void compareKeys() throws Exception {
        for (LabelAllocator a1 : allocators) {
            for (LabelAllocator a2 : allocators) {
                Label l1 = a1.alloc("cde", "value1");
                assertTrue(l1.compareKeys(a2.alloc("abc", "value2")) > 0);
                assertTrue(l1.compareKeys(a2.alloc("def", "value2")) < 0);
                assertTrue(l1.compareKeys(a2.alloc("cde", "value2")) == 0);
            }
        }
    }

    @Test
    public void testToString() throws Exception {
        for (LabelAllocator a : allocators) {
            {
                StringBuilder sb = new StringBuilder();
                a.alloc("some", "label").toString(sb);
                assertEquals("some='label'", sb.toString());
            }
            {
                StringBuilder sb = new StringBuilder();
                a.alloc("another", "label with spaces").toString(sb);
                assertEquals("another='label with spaces'", sb.toString());
            }
            {
                StringBuilder sb = new StringBuilder();
                a.alloc("even", "label (with) [some,punctuations]").toString(sb);
                assertEquals("even='label (with) [some,punctuations]'", sb.toString());
            }
        }
    }
}
