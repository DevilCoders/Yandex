package ru.yandex.ci.client.tsum;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import lombok.Value;

@Value
public class TsumPipeline {
    String id;
    ConfigurationType configurationType;
    boolean archived;
    int configurationVersion;
    List<String> manualResources;

    public enum ConfigurationType {
        CD,
        RELEASE,
        MT,
        PER_COMMIT,

        @JsonEnumDefaultValue
        UNKNOWN
    }

}
