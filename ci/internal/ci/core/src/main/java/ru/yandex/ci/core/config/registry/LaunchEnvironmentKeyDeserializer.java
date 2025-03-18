package ru.yandex.ci.core.config.registry;

import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.KeyDeserializer;

import ru.yandex.ci.core.launch.TaskVersion;

public class LaunchEnvironmentKeyDeserializer extends KeyDeserializer {

    @Override
    public TaskVersion deserializeKey(String key, DeserializationContext ctxt) {
        return TaskVersion.of(key);
    }
}
