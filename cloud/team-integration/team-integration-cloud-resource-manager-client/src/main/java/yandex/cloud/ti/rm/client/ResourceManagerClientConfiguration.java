package yandex.cloud.ti.rm.client;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.di.Configuration;

public class ResourceManagerClientConfiguration extends Configuration {

    private final @NotNull String applicationName;


    public ResourceManagerClientConfiguration(@NotNull String applicationName) {
        this.applicationName = applicationName;
    }


    @Override
    public void configure() {
        put(ResourceManagerClient.class, this::resourceManagerClient);
    }

    protected @NotNull ResourceManagerClient resourceManagerClient() {
        return ResourceManagerClientFactory.createResourceManagerClient(
                applicationName,
                get(ResourceManagerClientProperties.class).endpoint(),
                get(SystemAccountService.class)::getIamToken
        );
    }

}
