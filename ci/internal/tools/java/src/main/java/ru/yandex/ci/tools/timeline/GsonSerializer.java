package ru.yandex.ci.tools.timeline;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import org.ehcache.spi.serialization.Serializer;
import org.ehcache.spi.serialization.SerializerException;

import ru.yandex.ci.util.gson.CiGson;

public class GsonSerializer<T> implements Serializer<T> {
    private static final String CLASS_FIELD = "class";
    private static final String DATA_FIELD = "data";

    private final Gson gson = CiGson.instance();
    private final ClassLoader loader;

    public GsonSerializer(ClassLoader loader) {
        this.loader = loader;
    }

    @SuppressWarnings("unchecked")
    public static <T> Class<? extends Serializer<T>> asTypedSerializer() {
        return (Class) GsonSerializer.class;
    }

    @Override
    public ByteBuffer serialize(T object) throws SerializerException {
        var json = gson.toJsonTree(object);
        var container = new JsonObject();
        container.addProperty(CLASS_FIELD, object.getClass().getName());
        container.add("data", json);
        return ByteBuffer.wrap(gson.toJson(container).getBytes(StandardCharsets.UTF_8));
    }

    @Override
    public T read(ByteBuffer binary) throws SerializerException {
        try {
            var container = JsonParser.parseString(toString(binary)).getAsJsonObject();
            var className = container.get(CLASS_FIELD).getAsString();
            Class<?> valueClass = loader.loadClass(className);
            //noinspection unchecked
            return (T) gson.fromJson(container.get(DATA_FIELD), valueClass);
        } catch (ClassNotFoundException e) {
            throw new SerializerException(e);
        }
    }

    private String toString(ByteBuffer binary) {
        return StandardCharsets.UTF_8.decode(binary).toString();
    }

    @Override
    public boolean equals(T object, ByteBuffer binary) throws SerializerException {
        return serialize(object).equals(binary);
    }
}
