package yandex.cloud.team.integration.abc;

import io.grpc.StatusRuntimeException;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.TestAbcServiceClouds;

@DependsOn(AbcIntegrationScenarioSuite.class)
public class AlreadyCreatedScenario extends AbcIntegrationScenarioBase {

    @Override
    @Test
    public void main() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        getAbcIntegrationRepository().createAbcServiceCloud(abcServiceCloud);

        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createById(abcServiceCloud.abcServiceId(), null)
                )
                .withMessage("ALREADY_EXISTS: Cloud '%s' already created for ABC service id %d, slug '%s', abc folder '%s'",
                        abcServiceCloud.cloudId(),
                        abcServiceCloud.abcServiceId(),
                        abcServiceCloud.abcServiceSlug(),
                        abcServiceCloud.abcdFolderId()
                );
    }

}
