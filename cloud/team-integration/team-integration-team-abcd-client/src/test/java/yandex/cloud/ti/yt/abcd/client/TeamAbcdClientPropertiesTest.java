package yandex.cloud.ti.yt.abcd.client;

import java.time.Duration;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;
import yandex.cloud.ti.grpc.DefaultEndpointConfig;

public class TeamAbcdClientPropertiesTest {

    @Test
    public void loadTeamAbcdClientProperties() throws Exception {
        TeamAbcdClientProperties properties = ConfigLoader.loadConfigFromStringContent(TeamAbcdClientProperties.class, """
                endpoint:
                    host: host
                    port: 1
                    tls: true
                    maxRetries: 10
                    timeout: PT10S
                    tcpKeepaliveInterval: PT1M
                tvmId: 42
                """
        );
        Assertions.assertThat(properties)
                .isEqualTo(new TeamAbcdClientProperties(
                        new DefaultEndpointConfig(
                                "host",
                                1,
                                true,
                                10,
                                Duration.ofSeconds(10),
                                Duration.ofMinutes(1)
                        ),
                        42
                ));
    }

}
