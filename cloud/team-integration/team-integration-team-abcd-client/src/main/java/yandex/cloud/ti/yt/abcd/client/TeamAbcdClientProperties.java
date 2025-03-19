package yandex.cloud.ti.yt.abcd.client;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.grpc.DefaultEndpointConfig;

public record TeamAbcdClientProperties(
        @NotNull DefaultEndpointConfig endpoint,
        int tvmId
) {
}
