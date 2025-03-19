package yandex.cloud.dashboard.util;

import org.junit.Test;
import yandex.cloud.dashboard.util.reflection.Reflector;

import java.lang.reflect.Field;
import java.util.Map;

import static java.util.Arrays.asList;
import static java.util.stream.Collectors.toList;
import static org.junit.Assert.assertEquals;

/**
 * @author ssytnik
 */
public class ReflectorTest {

    @Test
    public void getFieldTypesMap() {
        assertEquals(Map.of("s", String.class, "i", int.class, "m", Map.class), Reflector.getFieldTypesMap(C.class));
    }

    @Test
    public void getFieldList() {
        assertEquals(asList("s", "i", "m"), Reflector.getFieldList(C.class).stream().map(Field::getName).collect(toList()));
    }

    static class C {
        static final String STATIC = "";
        String s;
        int i;
        Map m;
    }

    ;

}