package yandex.cloud.ti.abcd.adapter;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.abcd.provider.AbcdProvisionUpdate;
import yandex.cloud.ti.abcd.provider.MappedQuotaMetricUpdate;

record UpdateProvisionParameters(
        @NotNull String accountId,

        @NotNull String providerId,
        long abcServiceId,
        @NotNull String abcFolderId,

        @NotNull List<AbcdProvisionUpdate> provisionUpdates,
        @NotNull List<MappedQuotaMetricUpdate> mappedQuotaMetricUpdates
) {
}
