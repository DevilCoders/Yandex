package yandex.cloud.ti.tvm.client;

import java.util.Map;
import java.util.Set;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.client.tvm.TvmClient;
import yandex.cloud.iam.client.tvm.TvmClientFactory;
import yandex.cloud.iam.client.tvm.config.TvmClientConfig;

public class TestTvmClientConfiguration extends TvmClientConfiguration {

    @Override
    protected void configure() {
        super.configure();
        merge(new TvmClientPropertiesConfiguration(
                TestTvmClientConfiguration::createTvmClientConfig
        ));
    }

    @Override
    protected TvmClient tvmClient() {
        return TvmClientFactory.createStubTvmClient();
    }

    private static @NotNull TvmClientConfig createTvmClientConfig() {
        return new TvmClientConfig(
                1,
                false,
                "tvm_secret_1",
                Map.of(0, new TvmClientConfig.Subject("sa", null)),
                Set.of(0)
        );
    }

}
