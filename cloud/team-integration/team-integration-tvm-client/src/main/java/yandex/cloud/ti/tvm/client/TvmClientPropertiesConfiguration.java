package yandex.cloud.ti.tvm.client;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;
import yandex.cloud.iam.client.tvm.config.TvmClientConfig;

public class TvmClientPropertiesConfiguration extends Configuration {

    private final @NotNull Supplier<TvmClientConfig> tvmClientConfigSupplier;


    public TvmClientPropertiesConfiguration(
            @NotNull Supplier<TvmClientConfig> tvmClientConfigSupplier
    ) {
        this.tvmClientConfigSupplier = tvmClientConfigSupplier;
    }


    @Override
    public void configure() {
        put(TvmClientConfig.class, tvmClientConfigSupplier);
    }

}
