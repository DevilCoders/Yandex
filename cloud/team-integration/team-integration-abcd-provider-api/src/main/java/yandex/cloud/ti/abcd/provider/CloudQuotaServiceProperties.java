package yandex.cloud.ti.abcd.provider;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.grpc.DefaultEndpointConfig;

public record CloudQuotaServiceProperties(
        @NotNull String name,
        @NotNull DefaultEndpointConfig endpoint
) {
}
