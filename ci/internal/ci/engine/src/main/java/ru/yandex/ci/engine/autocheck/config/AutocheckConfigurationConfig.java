package ru.yandex.ci.engine.autocheck.config;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.config.ConfigIdEntry;

@Value
@SuppressWarnings("ReferenceEquality")
@JsonIgnoreProperties(ignoreUnknown = true)
public class AutocheckConfigurationConfig implements ConfigIdEntry<AutocheckConfigurationConfig> {
    @With(onMethod_ = @Override)
    @Getter(onMethod_ = @Override)
    String id;
    AutocheckPartitionsConfig partitions;
    boolean enabled;

    public AutocheckConfigurationConfig(String id, AutocheckPartitionsConfig partitions, boolean enabled) {
        this.id = id;
        this.partitions = partitions;
        this.enabled = enabled;
    }

    @JsonCreator
    public AutocheckConfigurationConfig(
            @JsonProperty("partitions") AutocheckPartitionsConfig partitions,
            @JsonProperty("enabled") boolean enabled
    ) {
        this.id = null;
        this.partitions = partitions;
        this.enabled = enabled;
    }
}
