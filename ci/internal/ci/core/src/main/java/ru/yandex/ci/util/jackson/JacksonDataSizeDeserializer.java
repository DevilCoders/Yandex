package ru.yandex.ci.util.jackson;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonToken;
import com.fasterxml.jackson.databind.BeanProperty;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.deser.ContextualDeserializer;
import com.google.common.base.Preconditions;
import org.springframework.util.unit.DataSize;

import ru.yandex.ci.util.FileUtils;

public class JacksonDataSizeDeserializer extends JsonDeserializer<DataSize> implements ContextualDeserializer {

    @Override
    public JsonDeserializer<?> createContextual(DeserializationContext ctxt, BeanProperty property) {
        return this;
    }

    @Override
    public DataSize deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        switch (p.getCurrentToken()) {
            case VALUE_STRING -> {
                // Probably read from YAML
                var value = p.getText();
                return FileUtils.parseFileSize(value);
            }
            case START_OBJECT -> {
                // Probably read from JSON
                p.nextToken();
                var bytesName = p.getText();
                Preconditions.checkState("bytes".equals(bytesName),
                        "Expect 'bytes' attribute, got %s", bytesName);

                p.nextToken();
                var bytes = p.getLongValue();

                p.nextToken();
                Preconditions.checkState(p.getCurrentToken() == JsonToken.END_OBJECT,
                        "Expect END_OBJECT, got %s", p.getCurrentToken());
                return DataSize.ofBytes(bytes);
            }
            default -> throw new IllegalStateException("Unknown token: " + p.getCurrentToken());
        }
    }
}
