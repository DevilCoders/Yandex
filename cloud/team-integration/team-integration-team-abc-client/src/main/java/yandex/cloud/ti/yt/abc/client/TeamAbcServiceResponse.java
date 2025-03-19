package yandex.cloud.ti.yt.abc.client;

import com.fasterxml.jackson.annotation.JsonProperty;
import org.jetbrains.annotations.NotNull;

record TeamAbcServiceResponse(
        @JsonProperty(required = true) long id,
        @JsonProperty(required = true) @NotNull String slug,
        @JsonProperty(required = true) @NotNull TeamAbcServiceNameResponse name
) {
}
