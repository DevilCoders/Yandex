package ru.yandex.ci.util.gson;

import java.io.IOException;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;

/**
 * Библиотека kikimr-repository использует для сериализации/десериализации Jackson. Добавление анотаций
 *
 * <pre>
 * \@JsonDeserialize(using = GsonJacksonDeserializer.class)
 * \@JsonSerialize(using = GsonJacksonSerializer.class)
 * </pre>
 * позволяет по прежнему использовать Gson
 */
public class GsonJacksonSerializer<T> extends JsonSerializer<T> {

    @Nullable
    private final Class<T> handledType;

    public GsonJacksonSerializer() {
        this.handledType = null;
    }

    public GsonJacksonSerializer(Class<T> handledType) {
        this.handledType = Objects.requireNonNull(handledType);
    }

    @Override
    public Class<T> handledType() {
        return handledType;
    }

    @Override
    public void serialize(T value, JsonGenerator gen, SerializerProvider serializers) throws IOException {
        gen.writeRawValue(CiGson.instance().toJson(value));
    }
}
