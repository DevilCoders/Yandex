package ru.yandex.ci.tools.potato;

import java.lang.reflect.Type;
import java.util.Map;

import javax.annotation.Nullable;

import ru.yandex.ci.util.gson.TypesafeType;

public class JsonMap<K, V> implements TypesafeType<Map<K, V>> {

    private final Class<K> keyClass;
    private final Class<V> valueClass;

    public JsonMap(Class<K> keyClass, Class<V> valueClass) {
        this.keyClass = keyClass;
        this.valueClass = valueClass;
    }

    public static <K, V> JsonMap<K, V> of(Class<K> keyClass, Class<V> valueClass) {
        return new JsonMap<>(keyClass, valueClass);
    }

    public static <V> JsonMap<String, V> ofStringTo(Class<V> valueClass) {
        return new JsonMap<>(String.class, valueClass);
    }

    @Override
    public Type[] getActualTypeArguments() {
        return new Type[]{keyClass, valueClass};
    }

    @Override
    public Type getRawType() {
        return Map.class;
    }

    @Override
    @Nullable
    public Type getOwnerType() {
        return null;
    }

}
