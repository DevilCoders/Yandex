package ru.yandex.ci.util.gson;

import java.io.IOException;
import java.time.Instant;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonToken;
import com.fasterxml.jackson.databind.BeanProperty;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.deser.ContextualDeserializer;
import com.fasterxml.jackson.datatype.jsr310.deser.InstantDeserializer;
import com.google.common.base.Preconditions;

public class GsonCompatibleInstantDeserializer extends JsonDeserializer<Instant> implements ContextualDeserializer {

    private static final String TOKEN_SECONDS = "seconds";
    private static final String TOKEN_NANOS = "nanos";

    @Override
    public JsonDeserializer<?> createContextual(DeserializationContext ctxt, BeanProperty property) {
        return this;
    }

    @Override
    public Instant deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        if (p.getCurrentToken() == JsonToken.START_OBJECT) {
            // Serialized by GSON (fields in any order)

            p.nextToken();
            var firstToken = p.getText();

            p.nextToken();
            var firstValue = p.getLongValue();

            p.nextToken();
            var secondToken = p.getText();

            p.nextToken();
            var secondValue = p.getLongValue();

            long seconds;
            int nanoseconds;
            if (TOKEN_SECONDS.equals(firstToken) && TOKEN_NANOS.equals(secondToken)) {
                seconds = firstValue;
                nanoseconds = (int) secondValue;
            } else if (TOKEN_NANOS.equals(firstToken) && TOKEN_SECONDS.equals(secondToken)) {
                seconds = secondValue;
                nanoseconds = (int) firstValue;
            } else {
                throw new IllegalStateException("Unexpected tokens: [" + firstToken + ", " + secondToken +
                        "]. Must be [" + TOKEN_SECONDS + ", " + TOKEN_NANOS + "]");
            }

            p.nextToken();
            Preconditions.checkState(p.getCurrentToken() == JsonToken.END_OBJECT, "Expect END_OBJECT");

            return Instant.ofEpochSecond(seconds, nanoseconds);
        } else {
            // Serialized using standard Jackson
            return InstantDeserializer.INSTANT.deserialize(p, ctxt);
        }
    }
}
