package yandex.cloud.ti.yt.abc.client;

import org.jetbrains.annotations.NotNull;

public class MockTeamAbcClientConfiguration extends TeamAbcClientConfiguration {

    public MockTeamAbcClientConfiguration(@NotNull String applicationName) {
        super(applicationName);
    }


    @Override
    public void configure() {
        super.configure();
        put(MockTeamAbcClient.class, this::mockTeamAbcClient);
    }

    @Override
    protected @NotNull TeamAbcClient teamAbcClient() {
        return new MockTeamAbcClient();
    }

    private @NotNull MockTeamAbcClient mockTeamAbcClient() {
        return (MockTeamAbcClient) get(TeamAbcClient.class);
    }

}
