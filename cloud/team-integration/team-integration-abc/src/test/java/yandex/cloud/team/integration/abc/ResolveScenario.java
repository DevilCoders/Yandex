package yandex.cloud.team.integration.abc;

import io.grpc.StatusRuntimeException;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.priv.team.integration.v1.PTIAS;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.TestAbcServiceClouds;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abc.client.TestTeamAbcServices;

@DependsOn(AbcIntegrationScenarioSuite.class)
public class ResolveScenario extends AbcIntegrationScenarioBase {

    @Test
    @Override
    public void main() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        getAbcIntegrationRepository().createAbcServiceCloud(abcServiceCloud);

        Assertions.assertThat(getAbcServiceClient().resolveByCloud(abcServiceCloud.cloudId()))
                .isNotNull()
                .extracting(PTIAS.ResolveResponse::getCloudId)
                .isEqualTo(abcServiceCloud.cloudId());
    }

    @Test
    public void testByAbcId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        getAbcIntegrationRepository().createAbcServiceCloud(abcServiceCloud);

        Assertions.assertThat(getAbcServiceClient().resolveById(abcServiceCloud.abcServiceId()))
                .isNotNull()
                .extracting(PTIAS.ResolveResponse::getCloudId)
                .isEqualTo(abcServiceCloud.cloudId());
    }

    @Test
    public void testByAbcSlug() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        getAbcIntegrationRepository().createAbcServiceCloud(abcServiceCloud);

        Assertions.assertThat(getAbcServiceClient().resolveBySlug(abcServiceCloud.abcServiceSlug()))
                .isNotNull()
                .extracting(PTIAS.ResolveResponse::getCloudId)
                .isEqualTo(abcServiceCloud.cloudId());
    }

    @Test
    public void testByAbcFolderId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        getAbcIntegrationRepository().createAbcServiceCloud(abcServiceCloud);

        var request = PTIAS.ResolveRequest.newBuilder()
                .setAbcFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThat(getAbcServiceClient().resolve(request))
                .isNotNull()
                .extracting(PTIAS.ResolveResponse::getCloudId)
                .isEqualTo(abcServiceCloud.cloudId());
    }

    @Test
    public void testEmptyRequest() {
        Assertions.assertThatThrownBy(
                        () -> getAbcServiceClient().resolve(PTIAS.ResolveRequest.getDefaultInstance()))
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("INVALID_ARGUMENT: Validation failed:\n" +
                        "  - abc: One of the options must be selected");
    }

    @Test
    public void testNotExistentById() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        Assertions.assertThatThrownBy(
                        () -> getAbcServiceClient().resolveById(abcService.id()))
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("NOT_FOUND: ABC service for id %d not found".formatted(
                        abcService.id()
                ));
    }

    @Test
    public void testNotExistentByCloudId() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        String cloudId = "cloud-id-" + abcService.id(); // todo need some method to generate "next" cloud id
        Assertions.assertThatThrownBy(
                        () -> getAbcServiceClient().resolveByCloud(cloudId))
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("NOT_FOUND: ABC service for cloud '%s' not found".formatted(
                        cloudId
                ));
    }

    @Test
    public void testNotExistentBySlug() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        Assertions.assertThatThrownBy(
                        () -> getAbcServiceClient().resolveBySlug(abcService.slug()))
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("NOT_FOUND: ABC service for slug '%s' not found".formatted(
                        abcService.slug()
                ));
    }

}
