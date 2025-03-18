package ru.yandex.monlib.metrics.encode.json;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.function.Consumer;

import org.junit.Test;

import static org.junit.Assert.assertEquals;


/**
 * @author Sergey Polovko
 */
public class JsonWriterTest {

    @Test
    public void empty() throws Exception {
        String result = writeToString(w -> {});
        assertEquals("", result);
    }

    @Test
    public void strings() throws Exception {
        String result1 = writeToString(w -> {
            w.stringValue(bytes("simple"));
        });
        assertEquals("\"simple\"", result1);

        String result2 = writeToString(w -> {
            w.stringValue(bytes("long string with spaces inside"));
        });
        assertEquals("\"long string with spaces inside\"", result2);
    }

    @Test
    public void numbers() throws Exception {
        String longs = writeToString(w -> {
            w.arrayBegin();
            w.numberValue(Long.MIN_VALUE);
            w.numberValue(-1);
            w.numberValue(0);
            w.numberValue(Long.MAX_VALUE);
            w.arrayEnd();
        });
        String longsExpected = String.format("[%s,%s,%s,%s]", Long.MIN_VALUE, -1L, 0L, Long.MAX_VALUE);
        assertEquals(longsExpected, longs);

        String doubles = writeToString(w -> {
            w.arrayBegin();
            w.numberValue(Double.MIN_VALUE);
            w.numberValue(Double.NEGATIVE_INFINITY);
            w.numberValue(Double.NaN);
            w.numberValue(0.0);
            w.numberValue((double) (1L << 53));
            w.numberValue((double) (1L << 53 + 1));
            w.numberValue(Double.POSITIVE_INFINITY);
            w.numberValue(Double.MAX_VALUE);
            w.arrayEnd();
        });
        String doublesExpected = String.format("[%s,%s,%s,%s,%s,%s,%s,%s]",
            Double.MIN_VALUE,
            Double.NEGATIVE_INFINITY,
            Double.NaN,
            0.0,
            (double) (1L << 53),
            (double) (1L << 53 + 1),
            Double.POSITIVE_INFINITY,
            Double.MAX_VALUE);
        assertEquals(doublesExpected, doubles);
    }

    @Test
    public void emptyObject() throws Exception {
        String result = writeToString(w -> {
            w.objectBegin();
            w.objectEnd();
        });
        assertEquals("{}", result);
    }

    @Test
    public void nonEmptyObject() throws Exception {
        String result1 = writeToString(w -> {
            w.objectBegin();
            w.key(bytes("one")); w.numberValue(1);
            w.objectEnd();
        });
        assertEquals(
            "{" +
                "\"one\":1" +
            "}", result1);

        String result2 = writeToString(w -> {
            w.objectBegin();
            w.key(bytes("string")); w.stringValue(bytes("some string"));
            w.key(bytes("long")); w.numberValue(123456);
            w.key(bytes("double")); w.numberValue(3.14159);
            w.key(bytes("array"));
            {
                w.arrayBegin();
                w.stringValue(bytes("one"));
                w.stringValue(bytes("two"));
                w.stringValue(bytes("three"));
                w.arrayEnd();
            }
            w.objectEnd();
        });
        assertEquals(
            "{" +
                "\"string\":\"some string\"," +
                "\"long\":123456," +
                "\"double\":3.14159," +
                "\"array\":[" +
                    "\"one\"," +
                    "\"two\"," +
                    "\"three\"" +
                "]" +
            "}", result2);
    }

    @Test
    public void emptyArray() throws Exception {
        String result = writeToString(w -> {
            w.arrayBegin();
            w.arrayEnd();
        });
        assertEquals("[]", result);
    }

    @Test
    public void arrayOfArrays() throws Exception {
        String result1 = writeToString(w -> {
            w.arrayBegin();
            {
                w.arrayBegin();
                w.arrayEnd();
            }
            w.arrayEnd();
        });
        assertEquals("[[]]", result1);

        String result2 = writeToString(w -> {
            w.arrayBegin();
            {
                w.arrayBegin();
                w.arrayEnd();
            }
            {
                w.arrayBegin();
                w.arrayEnd();
            }
            w.arrayEnd();
        });
        assertEquals("[[],[]]", result2);

        String result3 = writeToString(w -> {
            w.arrayBegin();
            {
                w.arrayBegin();
                {
                    w.arrayBegin();
                    w.arrayEnd();
                }
                w.arrayEnd();
            }
            {
                w.arrayBegin();
                {
                    w.arrayBegin();
                    w.arrayEnd();
                }
                {
                    w.arrayBegin();
                    w.arrayEnd();
                }
                w.arrayEnd();
            }
            w.arrayEnd();
        });
        assertEquals("[[[]],[[],[]]]", result3);
    }

    @Test
    public void arrayOfObjects() throws Exception {
        String result1 = writeToString(w -> {
            w.arrayBegin();
            {
                w.objectBegin();
                w.objectEnd();
            }
            w.arrayEnd();
        });
        assertEquals("[{}]", result1);

        String result2 = writeToString(w -> {
            w.arrayBegin();
            {
                w.objectBegin();
                w.objectEnd();
            }
            {
                w.objectBegin();
                w.objectEnd();
            }
            w.arrayEnd();
        });
        assertEquals("[{},{}]", result2);
    }

    @Test
    public void nonEmptyArray() throws Exception {
        String result1 = writeToString(w -> {
            w.arrayBegin();
            w.stringValue(bytes("one"));
            w.arrayEnd();
        });
        assertEquals("[\"one\"]", result1);

        String result2 = writeToString(w -> {
            w.arrayBegin();
            w.stringValue(bytes("one"));
            w.numberValue(2);
            w.numberValue(3.0);
            w.arrayEnd();
        });
        assertEquals("[\"one\",2,3.0]", result2);

        String result3 = writeToString(w -> {
            w.arrayBegin();
            w.stringValue(bytes("one"));
            w.numberValue(2);
            w.numberValue(3.0);
            {
                w.arrayBegin();
                w.arrayEnd();
            }
            {
                w.objectBegin();
                w.objectEnd();
            }
            w.arrayEnd();
        });
        assertEquals("[\"one\",2,3.0,[],{}]", result3);

        String result4 = writeToString(w -> {
            w.arrayBegin();
            w.stringValue(bytes("one"));
            w.numberValue(2);
            w.numberValue(3.0);
            {
                w.arrayBegin();
                w.stringValue(bytes("four"));
                w.arrayEnd();
            }
            {
                w.objectBegin();
                w.key(bytes("five"));
                w.numberValue(6);
                w.objectEnd();
            }
            w.arrayEnd();
        });
        assertEquals("[\"one\",2,3.0,[\"four\"],{\"five\":6}]", result4);
    }

    private static byte[] bytes(String one) {
        return one.getBytes(StandardCharsets.UTF_8);
    }

    private static String writeToString(Consumer<JsonWriter> consumer) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        try (JsonWriter w = new JsonWriter(out, 1024)) {
            consumer.accept(w);
        }
        return new String(out.toByteArray(), StandardCharsets.UTF_8);
    }
}
