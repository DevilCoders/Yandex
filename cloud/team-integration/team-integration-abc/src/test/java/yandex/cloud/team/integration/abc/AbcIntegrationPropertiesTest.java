package yandex.cloud.team.integration.abc;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.config.ConfigLoader;

public class AbcIntegrationPropertiesTest {

    @Test
    public void loadResourceManagerProperties() throws Exception {
        AbcIntegrationProperties properties = ConfigLoader.loadConfigFromStringContent(AbcIntegrationProperties.class, """
                abcServiceCloudOrganizationId: org1
                """
        );
        Assertions.assertThat(properties)
                .isEqualTo(new AbcIntegrationProperties(
                        "org1"
                ));
    }

}
