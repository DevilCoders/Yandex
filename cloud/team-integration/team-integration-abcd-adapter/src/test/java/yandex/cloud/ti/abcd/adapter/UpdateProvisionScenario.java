package yandex.cloud.ti.abcd.adapter;

import io.grpc.StatusRuntimeException;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.team.integration.abc.FakeResolveServiceFacade;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.TestAbcServiceClouds;
import yandex.cloud.ti.abcd.provider.TestAbcdProviders;

import ru.yandex.intranet.d.backend.service.provider_proto.AccountsSpaceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.Amount;
import ru.yandex.intranet.d.backend.service.provider_proto.CompoundAccountsSpaceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.CompoundResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.PassportUID;
import ru.yandex.intranet.d.backend.service.provider_proto.ProvisionRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceSegmentKey;
import ru.yandex.intranet.d.backend.service.provider_proto.StaffLogin;
import ru.yandex.intranet.d.backend.service.provider_proto.UpdateProvisionRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.UserID;

@DependsOn(AbcdAdapterScenarioSuite.class)
public class UpdateProvisionScenario extends AbcdAdapterScenarioBase {

    @Test
    @Override
    public void main() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = UpdateProvisionRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setAuthor(UserID.newBuilder()
                        .setPassportUid(PassportUID.newBuilder()
                                .setPassportUid(TestAbcdAdapters.templatePassportUid(1))
                        )
                        .setStaffLogin(StaffLogin.newBuilder()
                                .setStaffLogin(TestAbcdAdapters.templateStaffLogin(1))
                        )
                )
                .addUpdatedProvisions(ProvisionRequest.newBuilder()
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
                                .setValue(42L)
                                .setUnitKey("abcd-unit-key-2")
                        )
                )
                .build();
        var response = getAccountsServiceClient().updateProvision(request);
        Assertions.assertThat(response.getProvisionsList()).hasSize(2);
        Assertions.assertThat(response.hasAccountsSpaceKey()).isFalse();
        Assertions.assertThat(response.hasAccountVersion()).isFalse();
    }

    @Test
    public void updateProvision_withEmptyAccountsSpaceKey() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = UpdateProvisionRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setAuthor(UserID.newBuilder()
                        .setPassportUid(PassportUID.newBuilder()
                                .setPassportUid(TestAbcdAdapters.templatePassportUid(1))
                        )
                        .setStaffLogin(StaffLogin.newBuilder()
                                .setStaffLogin(TestAbcdAdapters.templateStaffLogin(1))
                        )
                )
                .setAccountsSpaceKey(AccountsSpaceKey.newBuilder()
                        .setCompoundKey(CompoundAccountsSpaceKey.newBuilder())
                )
                .build();
        var response = getAccountsServiceClient().updateProvision(request);
        Assertions.assertThat(response.getProvisionsList()).hasSize(2);
    }

    @Test
    public void updateProvision_withNonEmptyAccountsSpaceKey() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        var request = UpdateProvisionRequest.newBuilder()
                .setAccountId(abcServiceCloud.cloudId())
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setAuthor(UserID.newBuilder()
                        .setPassportUid(PassportUID.newBuilder()
                                .setPassportUid(TestAbcdAdapters.templatePassportUid(1))
                        )
                        .setStaffLogin(StaffLogin.newBuilder()
                                .setStaffLogin(TestAbcdAdapters.templateStaffLogin(1))
                        )
                )
                .setAccountsSpaceKey(AccountsSpaceKey.newBuilder()
                        .setCompoundKey(CompoundAccountsSpaceKey.newBuilder()
                                .addResourceSegmentKeys(ResourceSegmentKey.newBuilder()
                                        .setResourceSegmentationKey("segmentation")
                                        .setResourceSegmentKey("segment")
                                )
                        )
                )
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAccountsServiceClient().updateProvision(request))
                .withMessageEndingWith("INVALID_ARGUMENT: accounts_space_key is not supported");
    }

}
