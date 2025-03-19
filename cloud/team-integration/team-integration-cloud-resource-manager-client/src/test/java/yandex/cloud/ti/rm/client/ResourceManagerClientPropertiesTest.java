package yandex.cloud.ti.rm.client;

import java.time.Duration;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;
import yandex.cloud.ti.grpc.DefaultEndpointConfig;

public class ResourceManagerClientPropertiesTest {

    @Test
    public void loadResourceManagerProperties() throws Exception {
        ResourceManagerClientProperties properties = ConfigLoader.loadConfigFromStringContent(ResourceManagerClientProperties.class, """
                endpoint:
                    host: host
                    port: 1
                    tls: true
                    maxRetries: 10
                    timeout: PT10S
                    tcpKeepaliveInterval: PT1M
                """
        );
        Assertions.assertThat(properties)
                .isEqualTo(new ResourceManagerClientProperties(
                        new DefaultEndpointConfig(
                                "host",
                                1,
                                true,
                                10,
                                Duration.ofSeconds(10),
                                Duration.ofMinutes(1)
                        )
                ));
    }

}
