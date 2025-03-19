package yandex.cloud.ti.abcd.provider;

import java.time.Duration;
import java.util.List;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;
import yandex.cloud.ti.grpc.DefaultEndpointConfig;

public class AbcdProviderPropertiesTest {

    @Test
    public void loadAbcdProviderProperties() throws Exception {
        AbcdProviderProperties properties = ConfigLoader.loadConfigFromStringContent(AbcdProviderProperties.class, """
                id: abcdProvider1
                name: abcdProvider1 name
                quotaService:
                    name: quotaService1
                    endpoint:
                        host: host
                        port: 1
                        tls: true
                        maxRetries: 10
                        timeout: PT10S
                        tcpKeepaliveInterval: PT1M
                quotaQueryThreads: 5
                mappedQuotaMetrics:
                    -   name: cloudQuotaMetric1
                        abcdResource:
                            resourceKey:
                                resourceTypeKey: resourceTypeKey1
                            unitKey: resourceTypeKey1.unit
                """
        );
        Assertions.assertThat(properties)
                .isEqualTo(new AbcdProviderProperties(
                        "abcdProvider1",
                        "abcdProvider1 name",
                        new CloudQuotaServiceProperties(
                                "quotaService1",
                                new DefaultEndpointConfig(
                                        "host",
                                        1,
                                        true,
                                        10,
                                        Duration.ofSeconds(10),
                                        Duration.ofMinutes(1)
                                )
                        ),
                        5,
                        List.of(
                                new MappedQuotaMetric(
                                        "cloudQuotaMetric1",
                                        new MappedAbcdResource(
                                                new AbcdResourceKey(
                                                        "resourceTypeKey1",
                                                        List.of()
                                                ),
                                                "resourceTypeKey1.unit"
                                        )
                                )
                        )
                ));
    }

    @Test
    public void loadAbcdProviderPropertiesWithEmptyMappedQuotaMetrics() throws Exception {
        AbcdProviderProperties properties = ConfigLoader.loadConfigFromStringContent(AbcdProviderProperties.class, """
                id: abcdProvider1
                name: abcdProvider1 name
                quotaService:
                    name: quotaService1
                    endpoint:
                        host: host
                        port: 1
                        tls: true
                        maxRetries: 10
                        timeout: PT10S
                        tcpKeepaliveInterval: PT1M
                mappedQuotaMetrics:
                """
        );
        Assertions.assertThat(properties)
                .isEqualTo(new AbcdProviderProperties(
                        "abcdProvider1",
                        "abcdProvider1 name",
                        new CloudQuotaServiceProperties(
                                "quotaService1",
                                new DefaultEndpointConfig(
                                        "host",
                                        1,
                                        true,
                                        10,
                                        Duration.ofSeconds(10),
                                        Duration.ofMinutes(1)
                                )
                        ),
                        1,
                        List.of()
                ));
    }

}
