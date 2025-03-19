package yandex.cloud.ti.abcd.adapter;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.abcd.provider.CloudQuotaServiceClientFactory;
import yandex.cloud.ti.abcd.provider.mdb.MdbCloudQuotaServiceClientFactory;
import yandex.cloud.ti.abcd.provider.ydb.YdbCloudQuotaServiceClientFactory;

final class CloudQuotaServiceClientFactories {
    // todo make non-static
    //  move into provider-api module
    //  register known factories, maybe even in configuration as []

    private static final @NotNull List<CloudQuotaServiceClientFactory> factories = List.of(
            new MdbCloudQuotaServiceClientFactory(),
            new YdbCloudQuotaServiceClientFactory()
    );

    // This map serves two purposes:
    //  * speed-ups getFactory() a little
    //  * ensures that CloudQuotaServiceClientFactory::getQuotaServiceName is unique
    private static final @NotNull Map<String, CloudQuotaServiceClientFactory> factoriesByQuotaServiceName = factories.stream()
            .collect(Collectors.toUnmodifiableMap(
                    CloudQuotaServiceClientFactory::getQuotaServiceName,
                    it -> it
            ));


    public static @NotNull CloudQuotaServiceClientFactory getFactoryByQuotaServiceName(
            @NotNull String quotaServiceName
    ) {
        CloudQuotaServiceClientFactory factory = factoriesByQuotaServiceName.get(quotaServiceName);
        if (factory == null) {
            // todo better exception type and message
            throw new IllegalArgumentException("unknown quota service %s".formatted(
                    quotaServiceName
            ));
        }
        return factory;
    }


    private CloudQuotaServiceClientFactories() {
    }

}
