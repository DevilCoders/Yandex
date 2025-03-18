package ru.yandex.ci.client.blackbox.jackson;

import java.io.IOException;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonToken;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;

public class StringToSetDeserializer extends StdDeserializer<Set<String>> {

    protected StringToSetDeserializer() {
        super(Set.class);
    }

    @Nullable
    @Override
    public Set<String> deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        if (p.hasToken(JsonToken.VALUE_STRING)) {
            var value = p.getText();
            if (value.isEmpty()) {
                return Set.of();
            } else {
                return new LinkedHashSet<>(List.of(value.split("\\s+")));
            }
        }
        return null;
    }
}
