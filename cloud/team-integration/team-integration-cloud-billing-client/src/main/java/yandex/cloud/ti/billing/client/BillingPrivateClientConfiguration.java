package yandex.cloud.ti.billing.client;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.di.Configuration;

public class BillingPrivateClientConfiguration extends Configuration {

    private final @NotNull String applicationName;


    public BillingPrivateClientConfiguration(@NotNull String applicationName) {
        this.applicationName = applicationName;
    }


    @Override
    public void configure() {
        put(BillingPrivateClient.class, this::createBillingPrivateClient);
    }

    private @Nullable BillingPrivateClient createBillingPrivateClient() {
        var config = get(BillingPrivateConfig.class);
        if (config == null) {
            return null;
        }
        return new BillingPrivateClientImpl(applicationName, config, get(SystemAccountService.class)::getIamToken);
    }

}
