package yandex.cloud.ti.yt.abc.client;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;
import yandex.cloud.iam.client.tvm.TvmClient;

public class TeamAbcClientConfiguration extends Configuration {

    private final @NotNull String applicationName;


    public TeamAbcClientConfiguration(
            @NotNull String applicationName
    ) {
        this.applicationName = applicationName;
    }


    @Override
    public void configure() {
        put(TeamAbcClient.class, this::teamAbcClient);
    }

    protected @NotNull TeamAbcClient teamAbcClient() {
        return new TeamAbcClientImpl(
                applicationName,
                get(TeamAbcClientProperties.class),
                get(TvmClient.class)
        );
    }

}
