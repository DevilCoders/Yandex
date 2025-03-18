package ru.yandex.ci.util.jackson;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import org.springframework.util.unit.DataSize;

public class JacksonDataSizeSerializer extends JsonSerializer<DataSize> {

    @Override
    public void serialize(DataSize value, JsonGenerator gen, SerializerProvider serializers) throws IOException {
        gen.writeString(value.toBytes() + " B");
    }
}
