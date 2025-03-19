package yandex.cloud.ti.yt.abc.client;

import java.time.Duration;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;

public class TeamAbcClientPropertiesTest {

    @Test
    public void loadTeamAbcClientProperties() throws Exception {
        TeamAbcClientProperties properties = ConfigLoader.loadConfigFromStringContent(TeamAbcClientProperties.class, """
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
                .isEqualTo(new TeamAbcClientProperties(
                        "host",
                        10,
                        1,
                        Duration.ofMinutes(1),
                        Duration.ofSeconds(10),
                        true,
                        42
                ));
    }

}
