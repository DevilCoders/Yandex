package ru.yandex.ci.client.juggler;

import java.time.Instant;

import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import com.fasterxml.jackson.databind.module.SimpleModule;

public class JugglerModule extends SimpleModule {
    public JugglerModule() {
        super("juggler");

        addSerializer(Instant.class, new JugglerInstantSerializer());
        addDeserializer(Instant.class, new JugglerInstantDeserializer());
        setNamingStrategy(PropertyNamingStrategies.SNAKE_CASE);
    }
}
