package yandex.cloud.ti.grpc;

import java.time.Duration;

import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.ClientConfig;

@Value
public class DefaultEndpointConfig implements ClientConfig {

    @NotNull String host;
    int port;
    boolean tls;
    int maxRetries;
    @NotNull Duration timeout;
    @NotNull Duration tcpKeepaliveInterval;

}
