package yandex.cloud.dashboard.util.reflection;

import lombok.SneakyThrows;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

/**
 * @author ssytnik
 */
public class Reflector {
    private static final Map<Class, Map<String, Class>> fieldTypesMapByClass = new ConcurrentHashMap<>();
    private static final Map<Class, List<Field>> fieldListByClass = new ConcurrentHashMap<>();

    public static Map<String, Class> getFieldTypesMap(Class clazz) {
        return fieldTypesMapByClass.computeIfAbsent(clazz, __ ->
                Arrays.stream(clazz.getDeclaredFields())
                        .filter(f -> !Modifier.isStatic(f.getModifiers()))
                        .filter(f -> !f.getName().startsWith("this$"))
                        .collect(Collectors.toMap(Field::getName, Field::getType)));
    }

    public static List<Field> getFieldList(Class clazz) {
        return fieldListByClass.computeIfAbsent(clazz, __ ->
                Arrays.stream(clazz.getDeclaredFields())
                        .filter(f -> !Modifier.isStatic(f.getModifiers()))
                        .filter(f -> !f.getName().startsWith("this$"))
                        .peek(f -> f.setAccessible(true))
                        .collect(Collectors.toList()));
    }

    @SneakyThrows
    public static Object get(Field f, Object o) {
        return f.get(o);
    }
}
