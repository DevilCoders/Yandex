package yandex.cloud.ti.abcd.adapter;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.team.integration.abc.FakeResolveServiceFacade;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.TestAbcServiceClouds;
import yandex.cloud.ti.abcd.provider.TestAbcdProviders;

import ru.yandex.intranet.d.backend.service.provider_proto.Account;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsByFolderRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsRequest;

@DependsOn(AbcdAdapterScenarioSuite.class)
public class ListScenario extends AbcdAdapterScenarioBase {

    @Test
    @Override
    public void main() {
        FakeResolveServiceFacade.clearAbcServiceClouds();
        AbcServiceCloud abcServiceCloud1 = TestAbcServiceClouds.nextAbcServiceCloud();
        AbcServiceCloud abcServiceCloud2 = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud1);
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud2);
        var request = ListAccountsRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setWithProvisions(true)
                .build();
        var response = getAccountsServiceClient().listAccounts(request);
        Assertions.assertThat(response.getAccountsList().stream().map(Account::getAccountId).toList())
                .containsExactly(
                        abcServiceCloud1.cloudId(),
                        abcServiceCloud2.cloudId()
                );
    }

    @Test
    public void testListByFolderDefaultInstance() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = ListAccountsByFolderRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setWithProvisions(true)
                .build();
        var response = getAccountsServiceClient().listAccountsByFolder(request);
        Assertions.assertThat(response.getAccountsList().stream().map(Account::getAccountId).toList())
                .containsExactly(abcServiceCloud.cloudId());
    }

    @Test
    public void testListByFolderPageSize() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = ListAccountsByFolderRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setLimit(10)
                .build();
        var response = getAccountsServiceClient().listAccountsByFolder(request);
        Assertions.assertThat(response.getAccountsList())
                .hasSize(1);
    }

    @Test
    public void testListByFolder() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = ListAccountsByFolderRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setIncludeDeleted(true)
                .build();
        var response = getAccountsServiceClient().listAccountsByFolder(request);
        Assertions.assertThat(response.getAccountsList())
                .hasSize(1);
    }

}
