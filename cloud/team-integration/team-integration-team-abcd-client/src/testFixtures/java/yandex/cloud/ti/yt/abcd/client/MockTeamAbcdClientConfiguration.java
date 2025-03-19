package yandex.cloud.ti.yt.abcd.client;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public class MockTeamAbcdClientConfiguration extends TeamAbcdClientConfiguration {

    public MockTeamAbcdClientConfiguration(@NotNull String applicationName) {
        super(applicationName);
    }


    @Override
    public void configure() {
        put(MockTeamAbcdClient.class, this::mockTeamAbcdClient);
        super.configure();
    }

    private @NotNull MockTeamAbcdClient mockTeamAbcdClient() {
        return new MockTeamAbcdClient();
    }

    @Override
    protected @Nullable TeamAbcdClient teamAbcdClient() {
        return get(MockTeamAbcdClient.class);
    }

}
