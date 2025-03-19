package yandex.cloud.ti.abcd.adapter;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.ti.abcd.provider.MappedQuotaMetricValue;

public record AbcdAccount(
        @NotNull String id,
        @NotNull String providerId,

        long abcServiceId,
        @NotNull String abcFolderId,

        @Nullable String displayName,

        @NotNull List<MappedQuotaMetricValue> mappedQuotaMetricValues
) {

    public @NotNull AbcdAccount withProvisions(@NotNull List<MappedQuotaMetricValue> mappedQuotaMetricValues) {
        return new AbcdAccount(
                id,
                providerId,
                abcServiceId,
                abcFolderId,
                displayName,
                mappedQuotaMetricValues
        );
    }

}
