package yandex.cloud.ti.rm.client;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.grpc.DefaultEndpointConfig;

public record ResourceManagerClientProperties(
        @NotNull DefaultEndpointConfig endpoint
) {
}
