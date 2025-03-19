package yandex.cloud.ti.rm.client;

import org.jetbrains.annotations.NotNull;

public class MockResourceManagerClientConfiguration extends ResourceManagerClientConfiguration {

    public MockResourceManagerClientConfiguration(@NotNull String applicationName) {
        super(applicationName);
    }


    @Override
    public void configure() {
        put(MockResourceManagerClient.class, this::mockResourceManagerClient);
        super.configure();
    }

    private @NotNull MockResourceManagerClient mockResourceManagerClient() {
        return new MockResourceManagerClient();
    }

    @Override
    protected @NotNull ResourceManagerClient resourceManagerClient() {
        return get(MockResourceManagerClient.class);
    }

}
