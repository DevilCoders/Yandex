package yandex.cloud.ti.abcd.adapter;

import java.util.concurrent.ThreadLocalRandom;

import io.grpc.StatusRuntimeException;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.team.integration.abc.FakeResolveServiceFacade;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.TestAbcServiceClouds;
import yandex.cloud.ti.abcd.provider.TestAbcdProviders;
import yandex.cloud.ti.yt.abcd.client.TestTeamAbcdFolders;

import ru.yandex.intranet.d.backend.service.provider_proto.AccountsSpaceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.CompoundAccountsSpaceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.CreateAccountRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.PassportUID;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceSegmentKey;
import ru.yandex.intranet.d.backend.service.provider_proto.StaffLogin;
import ru.yandex.intranet.d.backend.service.provider_proto.UserID;

@DependsOn(AbcdAdapterScenarioSuite.class)
public class CreateScenario extends AbcdAdapterScenarioBase {

    @Test
    @Override
    public void main() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        String key = "TestAccountKey";
        String displayName = "TestAccountDisplayName";
        getMockTeamAbcdClient().addFolder(TestTeamAbcdFolders.templateAbcdFolder(
                abcServiceCloud.abcServiceId(),
                abcServiceCloud.abcdFolderId()
        ));
        boolean freeTier = ThreadLocalRandom.current().nextBoolean();
        var request = CreateAccountRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .setKey(key)
                .setDisplayName(displayName)
                .setFreeTier(freeTier)
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
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        getAccountsServiceClient().createAccount(request)
                )
                .withMessageStartingWith("ALREADY_EXISTS: abcd account already exists request=(%s, %s), exists=(%s, %s, %s)",
                        request.getAbcServiceId(),
                        request.getFolderId(),
                        abcServiceCloud.abcServiceId(),
                        abcServiceCloud.abcdFolderId(),
                        abcServiceCloud.cloudId()
                );
    }


    @Test
    public void createAccount_withRequiredFieldsOnly() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        FakeResolveServiceFacade.addAbcServiceCloud(abcServiceCloud);
        getMockTeamAbcdClient().addFolder(TestTeamAbcdFolders.templateAbcdFolder(
                abcServiceCloud.abcServiceId(),
                abcServiceCloud.abcdFolderId()
        ));
        var request = CreateAccountRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        getAccountsServiceClient().createAccount(request)
                )
                .withMessageStartingWith("ALREADY_EXISTS: abcd account already exists");
    }

    @Test
    public void createAccount_withoutProviderId() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        var request = CreateAccountRequest.newBuilder()
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        getAccountsServiceClient().createAccount(request)
                )
                .withMessageStartingWith("INVALID_ARGUMENT: provider_id is required");
    }

    @Test
    public void createAccount_withUnknownProviderId() {
        String providerId = TestAbcdProviders.nextAbcdProviderId();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        var request = CreateAccountRequest.newBuilder()
                .setProviderId(providerId)
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        getAccountsServiceClient().createAccount(request)
                )
                .withMessageStartingWith("NOT_FOUND: abcd provider %s not found".formatted(providerId));
    }

    @Test
    public void createAccount_withNonEmptyAccountsSpaceKey() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        var request = CreateAccountRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
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
                .isThrownBy(() ->
                        getAccountsServiceClient().createAccount(request)
                )
                .withMessageStartingWith("INVALID_ARGUMENT: accounts_space_key is not supported");
    }

    @Test
    public void createAccount_withDefaultAbcFolder_whenCloudDoesNotExist() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        getMockTeamAbcdClient().addFolder(TestTeamAbcdFolders.templateAbcdFolder(
                abcServiceCloud.abcServiceId(),
                abcServiceCloud.abcdFolderId()
        ));
        var request = CreateAccountRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        getAccountsServiceClient().createAccount(request)
                )
                .withMessageStartingWith("UNAVAILABLE: Internal server error");
    }

    @Test
    public void createAccount_withNonDefaultAbcFolder_whenCloudDoesNotExist() {
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();
        getMockTeamAbcdClient().addFolder(TestTeamAbcdFolders.templateAbcdFolder(
                abcServiceCloud.abcServiceId(),
                abcServiceCloud.abcdFolderId(),
                false
        ));
        var request = CreateAccountRequest.newBuilder()
                .setProviderId(TestAbcdProviders.testAbcdProvider().getId())
                .setAbcServiceId(abcServiceCloud.abcServiceId())
                .setFolderId(abcServiceCloud.abcdFolderId())
                .build();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        getAccountsServiceClient().createAccount(request)
                )
                .withMessageStartingWith("INVALID_ARGUMENT: creating abcd account for non-default abc folder is not supported");
    }

}
