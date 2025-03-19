package yandex.cloud.ti.yt.abc.client;

import java.time.Duration;

import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.ClientConfig;

@Value
public class TeamAbcClientProperties implements ClientConfig {

    @NotNull String host;
    int maxRetries;
    int port;
    @NotNull Duration tcpKeepaliveInterval;
    @NotNull Duration timeout;
    boolean tls;
    int tvmId;

}
