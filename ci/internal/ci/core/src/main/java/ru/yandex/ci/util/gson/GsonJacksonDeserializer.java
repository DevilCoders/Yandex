package ru.yandex.ci.util.gson;

import java.io.IOException;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.BeanProperty;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JavaType;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.deser.ContextualDeserializer;
import com.google.common.base.Preconditions;

/**
 * Библиотека kikimr-repository использует для сериализации/десериализации Jackson. Добавление анотаций
 *
 * <pre>
 * \@JsonDeserialize(using = GsonJacksonDeserializer.class)
 * \@JsonSerialize(using = GsonJacksonSerializer.class)
 * </pre>
 * позволяет по прежнему использовать Gson
 */
public class GsonJacksonDeserializer extends JsonDeserializer<Object> implements ContextualDeserializer {

    @Nullable
    private final JavaType type;

    public GsonJacksonDeserializer(@Nullable JavaType type) {
        this.type = type;
    }

    public GsonJacksonDeserializer() {
        this(null);
    }

    @Override
    public JsonDeserializer<?> createContextual(DeserializationContext ctxt, BeanProperty property) {
        JavaType javaType = ctxt.getContextualType() != null
                ? ctxt.getContextualType()
                : property.getMember().getType();
        return new GsonJacksonDeserializer(javaType);
    }

    @Override
    public Object deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        Preconditions.checkState(type != null);
        return CiGson.instance().fromJson(
                p.getCodec().readTree(p).toString(),
                type.getRawClass()
        );
    }
}
