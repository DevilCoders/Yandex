package ru.yandex.ci.util.gson;

import java.time.Instant;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.core.Version;
import com.fasterxml.jackson.databind.Module;
import com.fasterxml.jackson.databind.module.SimpleDeserializers;
import com.fasterxml.jackson.databind.module.SimpleSerializers;
import com.google.gson.JsonObject;

public class GsonToJacksonMapperModule extends Module {

    @Override
    public String getModuleName() {
        return "GsonToJacksonMapperModule";
    }

    @Override
    public Version version() {
        return Version.unknownVersion();
    }

    @Override
    public void setupModule(SetupContext context) {
        // Support form JsonObject serialization and deserialization
        // Support for Instant deserialization
        context.addDeserializers(new SimpleDeserializers(Map.of(
                JsonObject.class, new GsonJacksonDeserializer(),
                Instant.class, new GsonCompatibleInstantDeserializer()
        )));
        context.addSerializers(new SimpleSerializers(
                List.of(new GsonJacksonSerializer<>(JsonObject.class))));
    }
}
