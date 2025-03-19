package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.abc.repo.ListPage;
import yandex.cloud.ti.abcd.provider.AbcdResourceSegmentKey;
import yandex.cloud.ti.abcd.provider.MappedAbcdResource;
import yandex.cloud.ti.abcd.provider.MappedQuotaMetricValue;

import ru.yandex.intranet.d.backend.service.provider_proto.Account;
import ru.yandex.intranet.d.backend.service.provider_proto.AccountsPageToken;
import ru.yandex.intranet.d.backend.service.provider_proto.Amount;
import ru.yandex.intranet.d.backend.service.provider_proto.CompoundResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsByFolderResponse;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsResponse;
import ru.yandex.intranet.d.backend.service.provider_proto.Provision;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceSegmentKey;
import ru.yandex.intranet.d.backend.service.provider_proto.UpdateProvisionResponse;

final class AccountsServiceResponses {

    static @NotNull Account toAccount(
            @NotNull AbcdAccount source
    ) {
        Account.Builder builder = Account.newBuilder()
                .setAccountId(source.id())
                .setFolderId(source.abcFolderId())
                .setDisplayName(ProtoResponses.optionalField(source.displayName()));
        source.mappedQuotaMetricValues().stream()
                .map(AccountsServiceResponses::toProvision)
                .forEach(builder::addProvisions);
        return builder.build();
    }

    static @NotNull Provision toProvision(
            @NotNull MappedQuotaMetricValue source
    ) {
        return Provision.newBuilder()
                .setResourceKey(ResourceKey.newBuilder()
                        .setCompoundKey(toCompoundResourceKey(source.abcdResource()))
                )
                .setProvided(Amount.newBuilder()
                        .setUnitKey(source.abcdResource().unitKey())
                        .setValue(source.provided())
                )
                .setAllocated(Amount.newBuilder()
                        .setUnitKey(source.abcdResource().unitKey())
                        .setValue(source.allocated())
                )
                .build();
    }

    static @NotNull CompoundResourceKey toCompoundResourceKey(
            @NotNull MappedAbcdResource source
    ) {
        CompoundResourceKey.Builder builder = CompoundResourceKey.newBuilder()
                .setResourceTypeKey(source.resourceKey().resourceTypeKey());
        source.resourceKey().resourceSegmentKeys()
                .stream()
                .map(AccountsServiceResponses::toResourceSegmentKey)
                .forEach(builder::addResourceSegmentKeys);
        return builder.build();
    }

    static @NotNull ResourceSegmentKey toResourceSegmentKey(
            @NotNull AbcdResourceSegmentKey source
    ) {
        return ResourceSegmentKey.newBuilder()
                .setResourceSegmentationKey(source.segmentationKey())
                .setResourceSegmentKey(source.segmentKey())
                .build();
    }

    static @NotNull ListAccountsResponse toListAccountsResponse(
            @NotNull ListPage<AbcdAccount> source
    ) {
        ListAccountsResponse.Builder builder = ListAccountsResponse.newBuilder();
        source.items()
                .stream()
                .map(AccountsServiceResponses::toAccount)
                .forEach(builder::addAccounts);
        if (source.nextPageToken() != null) {
            builder.setNextPageToken(AccountsPageToken.newBuilder()
                    .setToken(source.nextPageToken())
            );
        }
        return builder.build();
    }

    static @NotNull ListAccountsByFolderResponse toListAccountsByFolderResponse(
            @NotNull ListPage<AbcdAccount> source
    ) {
        ListAccountsByFolderResponse.Builder builder = ListAccountsByFolderResponse.newBuilder();
        source.items()
                .stream()
                .map(AccountsServiceResponses::toAccount)
                .forEach(builder::addAccounts);
        if (source.nextPageToken() != null) {
            builder.setNextPageToken(AccountsPageToken.newBuilder()
                    .setToken(source.nextPageToken())
            );
        }
        return builder.build();
    }

    static @NotNull UpdateProvisionResponse toUpdateProvisionResponse(
            @NotNull AbcdAccount source
    ) {
        UpdateProvisionResponse.Builder builder = UpdateProvisionResponse.newBuilder();
        source.mappedQuotaMetricValues().stream()
                .map(AccountsServiceResponses::toProvision)
                .forEach(builder::addProvisions);
        return builder.build();
    }


    private AccountsServiceResponses() {
    }

}
