package yandex.cloud.ti.abcd.provider;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.priv.quota.PQ;

public class AbcdProvider {

    private final @NotNull String id;
    private final @NotNull String name;
    private final @NotNull CloudQuotaServiceClient cloudQuotaServiceClient;

    private final @NotNull Map<String, MappedQuotaMetric> mappedQuotaMetricsByName;
    private final @NotNull Map<MappedAbcdResource, MappedQuotaMetric> mappedQuotaMetricsByAbcdResource;

    private final @NotNull Executor quotaRequestExecutor;


    public AbcdProvider(
            @NotNull String id,
            @NotNull String name,
            @NotNull CloudQuotaServiceClient cloudQuotaServiceClient,
            int cloudQuotaQueryThreads,
            @NotNull Collection<MappedQuotaMetric> mappedQuotaMetrics
    ) {
        this.id = id;
        this.name = name;
        this.cloudQuotaServiceClient = cloudQuotaServiceClient;

        mappedQuotaMetricsByName = mappedQuotaMetrics.stream()
                .collect(Collectors.toUnmodifiableMap(
                        MappedQuotaMetric::name,
                        it -> it
                ));
        mappedQuotaMetricsByAbcdResource = mappedQuotaMetrics.stream()
                .collect(Collectors.toUnmodifiableMap(
                        MappedQuotaMetric::abcdResource,
                        it -> it
                ));

        quotaRequestExecutor = Executors.newFixedThreadPool(cloudQuotaQueryThreads);
    }


    public @NotNull String getId() {
        return id;
    }

    public @NotNull String getName() {
        return name;
    }


    public @NotNull Executor getQuotaRequestExecutor() {
        return quotaRequestExecutor;
    }

    public @NotNull List<MappedQuotaMetricValue> getQuotaMetrics(
            @NotNull String cloudId
    ) {
        List<PQ.QuotaMetric> quotaMetrics = cloudQuotaServiceClient.getQuotaMetrics(cloudId);
        return quotaMetrics.stream()
                .map(this::toMappedQuotaMetricValue)
                .filter(Objects::nonNull)
                .toList();
    }

    private @Nullable MappedQuotaMetricValue toMappedQuotaMetricValue(
            @NotNull PQ.QuotaMetric quotaMetric
    ) {
        MappedQuotaMetric mappedQuotaMetric = getMappedQuotaMetricByName(quotaMetric.getName());
        if (mappedQuotaMetric == null) {
            return null;
        }
        return new MappedQuotaMetricValue(
                mappedQuotaMetric.abcdResource(),
                quotaMetric.getLimit(),
                (long) Math.ceil(quotaMetric.getUsage())
        );
    }

    private @Nullable MappedQuotaMetric getMappedQuotaMetricByName(
            @NotNull String name
    ) {
        return mappedQuotaMetricsByName.get(name);
    }


    public void updateQuotaMetrics(
            @NotNull String cloudId,
            @NotNull Collection<MappedQuotaMetricUpdate> mappedQuotaMetricUpdates
    ) {
        List<PQ.MetricLimit> metricLimits = mappedQuotaMetricUpdates.stream()
                .map(this::toMetricLimit)
                .toList();
        cloudQuotaServiceClient.updateQuotaMetrics(cloudId, metricLimits);
    }

    private @NotNull PQ.MetricLimit toMetricLimit(
            @NotNull MappedQuotaMetricUpdate mappedQuotaMetricUpdate
    ) {
        MappedQuotaMetric mappedQuotaMetric = getMappedQuotaMetricByAbcdResource(mappedQuotaMetricUpdate.abcdResource());
        return PQ.MetricLimit.newBuilder()
                .setName(mappedQuotaMetric.name())
                .setLimit(mappedQuotaMetricUpdate.provided())
                .build();
    }

    private @NotNull MappedQuotaMetric getMappedQuotaMetricByAbcdResource(
            @NotNull MappedAbcdResource abcdResource
    ) {
        MappedQuotaMetric mappedQuotaMetric = mappedQuotaMetricsByAbcdResource.get(abcdResource);
        if (mappedQuotaMetric == null) {
            // todo the abcdResource is not registered, we do not know how to map it into the cloud quota
            //  add configuration option on how to handle unmapped abcd resources
            //      * return null - other quote updates will be applied
            //      * throw exception - the whole call fails

            // todo proper exception type, message and details
            throw new IllegalStateException();
        }
        return mappedQuotaMetric;
    }

}
