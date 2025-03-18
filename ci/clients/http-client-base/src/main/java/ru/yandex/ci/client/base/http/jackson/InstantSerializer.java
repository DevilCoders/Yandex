package ru.yandex.ci.client.base.http.jackson;

import java.io.IOException;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.Objects;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;

public class InstantSerializer extends JsonSerializer<Instant> {

    private final DateTimeFormatter formatter;

    public InstantSerializer(DateTimeFormatter formatter) {
        this.formatter = Objects.requireNonNull(formatter);
    }

    @Override
    public void serialize(Instant value, JsonGenerator gen, SerializerProvider serializers) throws IOException {
        gen.writeString(formatter.format(value));
    }
}
