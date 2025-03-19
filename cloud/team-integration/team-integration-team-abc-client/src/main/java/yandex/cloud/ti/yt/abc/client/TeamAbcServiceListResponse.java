package yandex.cloud.ti.yt.abc.client;

import com.fasterxml.jackson.annotation.JsonProperty;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

record TeamAbcServiceListResponse(
        @JsonProperty @Nullable String next,
        @JsonProperty @Nullable String previous,
        @JsonProperty(required = true) @NotNull TeamAbcServiceResponse[] results
) {
}
