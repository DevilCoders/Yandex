package yandex.cloud.ti.abcd.adapter;

import io.grpc.StatusRuntimeException;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.team.integration.abc.FakeResolveServiceFacade;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.TestAbcServiceClouds;
import yandex.cloud.ti.abcd.provider.TestAbcdProviders;
import yandex.cloud.ti.yt.abcd.client.TestTeamAbcdFolders;

import ru.yandex.intranet.d.backend.service.provider_proto.Amount;
import ru.yandex.intranet.d.backend.service.provider_proto.CompoundResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.GetAccountRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.Provision;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceSegmentKey;

@DependsOn(AbcdAdapterScenarioSuite.class)
public class GetScenario extends AbcdAdapterScenarioBase {

    @Test
    @Override
    public void main() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);

        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        var response = getAccountsServiceClient().getAccount(request);
        Assertions.assertThat(response.getAccountId()).isEqualTo(abcServiceCloud.cloudId());
        Assertions.assertThat(response.getKey()).isEmpty();
        Assertions.assertThat(response.getDisplayName()).isNotEmpty();
        Assertions.assertThat(response.getFolderId()).isEqualTo(abcServiceCloud.abcdFolderId());
        Assertions.assertThat(response.getDeleted()).isFalse();
        Assertions.assertThat(response.getProvisionsList()).isEmpty();
        Assertions.assertThat(response.hasAccountsSpaceKey()).isFalse();
        Assertions.assertThat(response.hasVersion()).isFalse();
        Assertions.assertThat(response.hasLastUpdate()).isFalse();
        Assertions.assertThat(response.getFreeTier()).isFalse();

    }

    @Test
    public void getAccount_withProvisions() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);

        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setWithProvisions(true)
                .build();
        var response = getAccountsServiceClient().getAccount(request);
        Assertions.assertThat(response.getAccountId()).isEqualTo(abcServiceCloud.cloudId());
        Assertions.assertThat(response.getProvisionsList()).containsExactlyInAnyOrder(
                Provision.newBuilder()
                        .setResourceKey(ResourceKey.newBuilder()
                                .setCompoundKey(CompoundResourceKey.newBuilder()
                                        .setResourceTypeKey("abcd-resource-key1")
                                )
                        )
                        .setProvided(Amount.newBuilder()
                                .setValue(2L)
                                .setUnitKey("abcd-unit-key-1")
                        )
                        .setAllocated(Amount.newBuilder()
                                .setValue(1L)
                                .setUnitKey("abcd-unit-key-1")
                        )
                        .build(),
                Provision.newBuilder()
                        .setResourceKey(ResourceKey.newBuilder()
                                .setCompoundKey(CompoundResourceKey.newBuilder()
                                        .setResourceTypeKey("abcd-resource-key2")
                                        .addResourceSegmentKeys(ResourceSegmentKey.newBuilder()
                                                .setResourceSegmentationKey("abcd-segmentation-key2")
                                                .setResourceSegmentKey("abcd-segment-key2")
                                        )
                                )
                        )
                        .setProvided(Amount.newBuilder()
                                .setValue(3L)
                                .setUnitKey("abcd-unit-key-2")
                        )
                        .setAllocated(Amount.newBuilder()
                                .setValue(2L)
                                .setUnitKey("abcd-unit-key-2")
                        )
                        .build()
        );
    }

    @Test
    public void testGetDefaultInstance() {
        var request = GetAccountRequest.getDefaultInstance();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().getAccount(request))
                .withMessageStartingWith("INVALID_ARGUMENT: account_id is required");
    }

    @Test
    public void testGetNonexistentAccount() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().getAccount(request))
                .withMessageStartingWith("NOT_FOUND: Account '%s' not found",
                        abcServiceCloud.cloudId()
                );
    }

    @Test
    public void testMismatchedAbcServiceId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(-abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().getAccount(request))
                .withMessageStartingWith("FAILED_PRECONDITION: Parameter 'abc_service_id' is invalid.");
    }

    @Test
    public void testMismatchedFolderId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(TestTeamAbcdFolders.nextAbcdFolderId(abcServiceCloud.abcServiceId()))
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().getAccount(request))
                .withMessageStartingWith("FAILED_PRECONDITION: Parameter 'folder_id' is invalid.");
    }

    @Test
    public void testMissingAbcServiceId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().getAccount(request))
                .withMessageStartingWith("INVALID_ARGUMENT: abc_service_id is required");
    }

    @Test
    public void testMissingFolderId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().getAccount(request))
                .withMessageStartingWith("INVALID_ARGUMENT: folder_id is required");
    }

    @Test
    public void testMissingProviderId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        var request = GetAccountRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().getAccount(request))
                .withMessageStartingWith("INVALID_ARGUMENT: provider_id is required");
    }

}
