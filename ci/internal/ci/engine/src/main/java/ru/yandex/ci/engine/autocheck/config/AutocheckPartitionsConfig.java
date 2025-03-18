package ru.yandex.ci.engine.autocheck.config;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.Value;

@Value
@JsonIgnoreProperties(ignoreUnknown = true)
public class AutocheckPartitionsConfig {
    int count;
}
