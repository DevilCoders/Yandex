package yandex.cloud.ti.abcd.provider;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.priv.quota.PQ;

public interface CloudQuotaServiceClient {

    @NotNull List<PQ.@NotNull QuotaMetric> getQuotaMetrics(
            @NotNull String cloudId
    );

    void updateQuotaMetrics(
            @NotNull String cloudId,
            @NotNull List<PQ.@NotNull MetricLimit> metricLimits
    );

}
