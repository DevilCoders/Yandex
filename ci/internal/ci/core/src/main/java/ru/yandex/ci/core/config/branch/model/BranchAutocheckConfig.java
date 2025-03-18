package ru.yandex.ci.core.config.branch.model;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.config.a.model.LargeAutostartConfig;

@Value
@Builder
@JsonDeserialize(builder = BranchAutocheckConfig.Builder.class)
public class BranchAutocheckConfig {

    @JsonProperty("pool")
    String pool;

    @Singular
    @JsonProperty("dirs")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> dirs;

    @JsonProperty("large-autostart")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<LargeAutostartConfig> largeAutostart;

    @JsonProperty("strong")
    boolean strong;

    public static class Builder {
        {
            this.largeAutostart = new ArrayList<>();
        }

        @JsonProperty("large-autostart")
        @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
        public void setLargeAutostart(List<Object> largeAutostart) {
            this.largeAutostart =
                    largeAutostart.stream().map(r -> {
                        if (r instanceof Map) {
                            return LargeAutostartConfig.fromMap((Map<?, ?>) r);
                        }
                        return new LargeAutostartConfig((String) r);
                    }).toList();
        }
    }
}
