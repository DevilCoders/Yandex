package yandex.cloud.ti.abcd.adapter;

import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.BadRequestException;
import yandex.cloud.ti.abcd.provider.AbcdProvisionUpdate;
import yandex.cloud.ti.abcd.provider.AbcdResourceKey;
import yandex.cloud.ti.abcd.provider.AbcdResourceSegmentKey;
import yandex.cloud.ti.abcd.provider.MappedAbcdResource;
import yandex.cloud.ti.abcd.provider.MappedQuotaMetricUpdate;

import ru.yandex.intranet.d.backend.service.provider_proto.AccountsSpaceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.CompoundResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.CreateAccountRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.GetAccountRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsByFolderRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ListAccountsRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ProvisionRequest;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceKey;
import ru.yandex.intranet.d.backend.service.provider_proto.ResourceSegmentKey;
import ru.yandex.intranet.d.backend.service.provider_proto.UpdateProvisionRequest;

final class AccountsServiceRequests {

    static @NotNull CreateAccountParameters toCreateAccountParameters(
            @NotNull CreateAccountRequest source
    ) {
        requireEmptyAccountsSpaceKey(source.getAccountsSpaceKey(), "accounts_space_key");
        // todo handle source.getAuthor()

        return new CreateAccountParameters(
                ProtoRequests.requiredField(source.getProviderId(), "provider_id"),
                ProtoRequests.requiredField(source.getAbcServiceId(), "abc_service_id"),
                ProtoRequests.requiredField(source.getFolderId(), "folder_id"),
                ProtoRequests.optionalField(source.getOperationId(), "operation_id"),
                ProtoRequests.optionalField(source.getKey(), "key"),
                ProtoRequests.optionalField(source.getDisplayName(), "display_name"),
                ProtoRequests.optionalField(source.getFreeTier(), "free_tier")
        );
    }

    static @NotNull GetAccountParameters toGetAccountParameters(
            @NotNull GetAccountRequest source
    ) {
        requireEmptyAccountsSpaceKey(source.getAccountsSpaceKey(), "accounts_space_key");
        return new GetAccountParameters(
                ProtoRequests.requiredField(source.getAccountId(), "account_id"),
                ProtoRequests.requiredField(source.getProviderId(), "provider_id"),
                ProtoRequests.requiredField(source.getAbcServiceId(), "abc_service_id"),
                ProtoRequests.requiredField(source.getFolderId(), "folder_id"),
                ProtoRequests.optionalField(source.getWithProvisions(), "with_provisions"),
                ProtoRequests.optionalField(source.getIncludeDeleted(), "include_deleted")
        );
    }

    static @NotNull ListAccountsParameters toListAccountsParameters(
            @NotNull ListAccountsRequest source
    ) {
        requireEmptyAccountsSpaceKey(source.getAccountsSpaceKey(), "accounts_space_key");
        return new ListAccountsParameters(
                ProtoRequests.requiredField(source.getProviderId(), "provider_id"),
                0,
                null,
                ProtoRequests.optionalField(source.getWithProvisions(), "with_provisions"),
                ProtoRequests.optionalField(source.getIncludeDeleted(), "include_deleted"),
                ProtoRequests.optionalField(source.getLimit(), 100, "limit"),
                ProtoRequests.optionalField(source.getPageToken().getToken(), "page_token")
        );
    }

    static @NotNull ListAccountsParameters toListAccountsParameters(
            @NotNull ListAccountsByFolderRequest source
    ) {
        requireEmptyAccountsSpaceKey(source.getAccountsSpaceKey(), "accounts_space_key");
        return new ListAccountsParameters(
                ProtoRequests.requiredField(source.getProviderId(), "provider_id"),
                ProtoRequests.requiredField(source.getAbcServiceId(), "abc_service_id"),
                ProtoRequests.requiredField(source.getFolderId(), "folder_id"),
                ProtoRequests.optionalField(source.getWithProvisions(), "with_provisions"),
                ProtoRequests.optionalField(source.getIncludeDeleted(), "include_deleted"),
                ProtoRequests.optionalField(source.getLimit(), 100, "limit"),
                ProtoRequests.optionalField(source.getPageToken().getToken(), "page_token")
        );
    }

    static @NotNull UpdateProvisionParameters toUpdateProvisionParameters(
            @NotNull UpdateProvisionRequest source
    ) {
        requireEmptyAccountsSpaceKey(source.getAccountsSpaceKey(), "accounts_space_key");
        // todo handle source.getOperationId()
        // todo handle source.getAuthor()
        // todo handle source.getKnownProvisionsList()
        return new UpdateProvisionParameters(
                ProtoRequests.requiredField(source.getAccountId(), "account_id"),
                ProtoRequests.requiredField(source.getProviderId(), "provider_id"),
                ProtoRequests.requiredField(source.getAbcServiceId(), "abc_service_id"),
                ProtoRequests.requiredField(source.getFolderId(), "folder_id"),
                toAbcdProvisionUpdates(source.getUpdatedProvisionsList()),
                toMappedQuotaMetricUpdates(source.getUpdatedProvisionsList())
        );
    }

    static @NotNull List<AbcdProvisionUpdate> toAbcdProvisionUpdates(
            @NotNull List<ProvisionRequest> source
    ) {
        return source.stream()
                .map(AccountsServiceRequests::toAbcdProvisionUpdate)
                .collect(Collectors.toUnmodifiableList());
    }

    static @NotNull AbcdProvisionUpdate toAbcdProvisionUpdate(
            @NotNull ProvisionRequest source
    ) {
        ResourceKey resourceKey = source.getResourceKey();
        CompoundResourceKey compoundKey = resourceKey.getCompoundKey();
        return new AbcdProvisionUpdate(
                new AbcdResourceKey(
                        ProtoRequests.requiredField(compoundKey.getResourceTypeKey(), "resource_type_key"),
                        toAbcdResourceSegmentKeys(compoundKey.getResourceSegmentKeysList())
                ),
                ProtoRequests.optionalField(source.getProvided().getValue(), "provided.value"),
                ProtoRequests.requiredField(source.getProvided().getUnitKey(), "provided.unit_key")
        );
    }

    static @NotNull List<MappedQuotaMetricUpdate> toMappedQuotaMetricUpdates(
            @NotNull Collection<ProvisionRequest> source
    ) {
        return source.stream()
                .map(AccountsServiceRequests::toMappedQuotaMetricUpdate)
                .toList();
    }

    static @NotNull MappedQuotaMetricUpdate toMappedQuotaMetricUpdate(
            @NotNull ProvisionRequest source
    ) {
        CompoundResourceKey compoundKey = source.getResourceKey().getCompoundKey();
        return new MappedQuotaMetricUpdate(
                new MappedAbcdResource(
                        new AbcdResourceKey(
                                ProtoRequests.requiredField(compoundKey.getResourceTypeKey(), "resource_type_key"),
                                toAbcdResourceSegmentKeys(compoundKey.getResourceSegmentKeysList())
                        ),
                        ProtoRequests.requiredField(source.getProvided().getUnitKey(), "provided.unit_key")
                ),
                ProtoRequests.optionalField(source.getProvided().getValue(), "provided.value")
        );
    }

    static @NotNull List<AbcdResourceSegmentKey> toAbcdResourceSegmentKeys(
            @NotNull Collection<ResourceSegmentKey> source
    ) {
        return source.stream()
                .map(AccountsServiceRequests::toAbcdResourceSegmentKey)
                .toList();
    }

    static @NotNull AbcdResourceSegmentKey toAbcdResourceSegmentKey(
            @NotNull ResourceSegmentKey source
    ) {
        return new AbcdResourceSegmentKey(
                ProtoRequests.requiredField(source.getResourceSegmentationKey(), "resource_segmentation_key"),
                ProtoRequests.requiredField(source.getResourceSegmentKey(), "resource_segment_key")
        );
    }


    private static void requireEmptyAccountsSpaceKey(
            @NotNull AccountsSpaceKey value,
            @NotNull String fieldName
    ) {
        if (value.getCompoundKey().getResourceSegmentKeysCount() != 0) {
            throw new UnsupportedFieldException(String.format("%s is not supported", fieldName));
        }
    }


    private AccountsServiceRequests() {
    }


    private static class UnsupportedFieldException extends BadRequestException {

        UnsupportedFieldException(String message) {
            super(message);
        }

    }

}
