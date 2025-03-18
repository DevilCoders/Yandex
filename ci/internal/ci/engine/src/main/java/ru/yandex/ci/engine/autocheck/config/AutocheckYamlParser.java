package ru.yandex.ci.engine.autocheck.config;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;

import ru.yandex.ci.core.config.YamlParsers;
import ru.yandex.ci.core.config.YamlPreProcessor;

public class AutocheckYamlParser {
    private static final ObjectMapper MAPPER = YamlParsers.buildMapper(PropertyNamingStrategies.SNAKE_CASE);

    private AutocheckYamlParser() {
    }

    public static AutocheckYamlConfig parse(String yamlSource) throws JsonProcessingException {
        var yaml = YamlPreProcessor.preprocess(yamlSource);
        return MAPPER.readValue(yaml, AutocheckYamlConfig.class);
    }

}
