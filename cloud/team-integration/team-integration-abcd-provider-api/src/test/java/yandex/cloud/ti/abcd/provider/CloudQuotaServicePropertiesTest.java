package yandex.cloud.ti.abcd.provider;

import java.time.Duration;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;
import yandex.cloud.ti.grpc.DefaultEndpointConfig;

public class CloudQuotaServicePropertiesTest {

    @Test
    public void loadCloudQuotaServiceProperties() throws Exception {
        CloudQuotaServiceProperties properties = ConfigLoader.loadConfigFromStringContent(CloudQuotaServiceProperties.class, """
                name: quotaService1
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
                .isEqualTo(new CloudQuotaServiceProperties(
                        "quotaService1",
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
