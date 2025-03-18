package ru.yandex.ci.core.config.a.model;

import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Getter;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.config.ConfigIdEntry;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@JsonDeserialize(builder = StageConfig.Builder.class)
public class StageConfig implements ConfigIdEntry<StageConfig> {

    public static final StageConfig IMPLICIT_STAGE =
            StageConfig.builder()
                    .id("single")
                    .title("Single")
                    .implicit(true)
                    .build();

    @JsonProperty
    @With(onMethod_ = @Override)
    @Getter(onMethod_ = @Override)
    String id;

    @JsonProperty
    @Nullable
    String title;

    @Nullable
    @JsonProperty
    DisplacementConfig displace;

    @JsonProperty
    boolean implicit;

    @JsonProperty
    boolean rollback;

    public String getTitle() {
        return Objects.requireNonNullElse(title, id);
    }
}
