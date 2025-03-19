package yandex.cloud.ti.yt.abcd.client;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.di.Configuration;
import yandex.cloud.iam.client.tvm.TvmClient;

public class TeamAbcdClientConfiguration extends Configuration {

    private final @NotNull String applicationName;


    public TeamAbcdClientConfiguration(@NotNull String applicationName) {
        this.applicationName = applicationName;
    }


    @Override
    public void configure() {
        put(TeamAbcdClient.class, this::teamAbcdClient);
    }

    protected @Nullable TeamAbcdClient teamAbcdClient() {
        return new TeamAbcdClientImpl(
                FolderServiceGrpcClientFactory.create(
                        applicationName,
                        get(TeamAbcdClientProperties.class),
                        get(TvmClient.class)
                )
        );
    }

}
