package ru.yandex.ci.util.jackson;

import java.io.IOException;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.BeanProperty;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JavaType;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.deser.ContextualDeserializer;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat;

import ru.yandex.ci.core.proto.ProtobufReflectionUtils;

public class JacksonProtobufDeserializer extends JsonDeserializer<Message> implements ContextualDeserializer {
    @Nullable
    private final JavaType type;

    public JacksonProtobufDeserializer(@Nullable JavaType type) {
        this.type = type;
    }

    private JacksonProtobufDeserializer() {
        this(null);
    }

    @Override
    public Message deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        if (type == null) {
            throw new RuntimeException("type is not set");
        }
        GeneratedMessageV3.Builder<?> builder = ProtobufReflectionUtils.getMessageBuilder(type.getRawClass());

        JsonFormat.parser().merge(p.readValueAsTree().toString(), builder);
        return builder.build();
    }

    @Override
    public JsonDeserializer<?> createContextual(DeserializationContext ctxt, BeanProperty property) {
        JavaType javaType = ctxt.getContextualType() != null
                ? ctxt.getContextualType()
                : property.getMember().getType();
        return new JacksonProtobufDeserializer(javaType);
    }
}
