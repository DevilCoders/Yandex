package yandex.cloud.team.integration.abc;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.di.Configuration;
import yandex.cloud.di.StaticDI;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.ti.billing.client.BillingPrivateClient;
import yandex.cloud.ti.billing.client.BillingPrivateConfig;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abc.client.TestTeamAbcServices;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;
import yandex.cloud.ti.yt.abcd.client.TestTeamAbcdFolders;

@DependsOn(AbcIntegrationScenarioSuite.class)
public class CreatedWithoutBillingScenario extends AbcIntegrationScenarioBase {

    @Override
    @Test
    public void main() {
        StaticDI.inject(new NoBillingConfiguration()).to("yandex.cloud.team.integration.abc");
    }

    @Test
    public void testCreateByAbcId() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);
        TeamAbcdFolder abcdFolder = TestTeamAbcdFolders.nextAbcdFolder(abcService.id());
        getMockTeamAbcdClient().addFolder(abcdFolder);

        var operation = getAbcServiceClient()
                .createById(abcService.id(), null);
        var response = waitOperationAndGetResponse(operation);
        Assertions.assertThat(response.getCloudId())
                .isNotEmpty();
        Assertions.assertThat(response.getDefaultFolderId())
                .isNotEmpty();

        // todo verify calls to getAbcIntegrationRepository()
    }

    private static class NoBillingConfiguration extends Configuration {

        @Override
        protected final void configure() {
            put(BillingPrivateConfig.class, () -> null);
            put(BillingPrivateClient.class, () -> null);
        }

    }

}
