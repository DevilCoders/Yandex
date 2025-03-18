package ru.yandex.ci.client.base.http.jackson;

import java.io.IOException;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.Objects;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;

public class InstantDeserializer extends JsonDeserializer<Instant> {

    private final DateTimeFormatter formatter;

    public InstantDeserializer(DateTimeFormatter formatter) {
        this.formatter = Objects.requireNonNull(formatter);
    }

    @Override
    public Instant deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        return Instant.from(formatter.parse(p.getText()));
    }
}
