package yandex.cloud.ti.abcd.provider;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.ClientConfig;

public interface CloudQuotaServiceClientFactory {

    @NotNull String getQuotaServiceName();

    @NotNull CloudQuotaServiceClient createCloudQuotaServiceClient(
            @NotNull String applicationName,
            @NotNull ClientConfig clientConfig,
            // todo create a separate TokenSupplier interface
            //  probably move "Bearer " part into it too
            @NotNull Supplier<String> tokenSupplier
    );

}
