package yandex.cloud.ti.yt.abc.client;

import com.fasterxml.jackson.annotation.JsonProperty;
import org.jetbrains.annotations.NotNull;

record TeamAbcServiceNameResponse(
        @JsonProperty(required = true) @NotNull String en,
        @JsonProperty(required = true) @NotNull String ru
) {
}
