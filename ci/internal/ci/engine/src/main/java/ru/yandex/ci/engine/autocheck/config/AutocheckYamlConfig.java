package ru.yandex.ci.engine.autocheck.config;

import java.util.LinkedHashMap;
import java.util.List;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

import ru.yandex.ci.core.config.ConfigUtils;

@Value
@JsonIgnoreProperties(ignoreUnknown = true)
public class AutocheckYamlConfig {
    List<AutocheckConfigurationConfig> configurations;

    @JsonCreator
    public AutocheckYamlConfig(
            @JsonProperty("configurations") LinkedHashMap<String, AutocheckConfigurationConfig> configurations
    ) {
        this.configurations = ConfigUtils.toList(configurations);
    }

    public AutocheckYamlConfig(List<AutocheckConfigurationConfig> configurations) {
        this.configurations = configurations;
    }
}
