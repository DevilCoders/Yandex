package yandex.cloud.team.integration.config;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;

public class ApplicationConfigTest {

    @Test
    public void loadApplicationConfig() {
        ApplicationConfig properties = ConfigLoader.loadConfig(ApplicationConfig.class, ApplicationConfigTest.class.getResource("/debug-application.yaml"));
        Assertions.assertThat(properties)
                .isNotNull();
    }

}
