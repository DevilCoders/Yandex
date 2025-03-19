package yandex.cloud.ti.abcd.provider;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.priv.quota.PQ;

public class TestAbcdProvider extends AbcdProvider {

    public TestAbcdProvider(
            @NotNull String id

    ) {
        super(
                id,
                id,
                new MockCloudQuotaServiceClient(),
                5,
                List.of(
                        new MappedQuotaMetric(
                                "service.quota1.unit1",
                                new MappedAbcdResource(
                                        new AbcdResourceKey(
                                                "abcd-resource-key1",
                                                null
                                        ),
                                        "abcd-unit-key-1"
                                )
                        ),
                        new MappedQuotaMetric(
                                "service.quota2.unit2",
                                new MappedAbcdResource(
                                        new AbcdResourceKey(
                                                "abcd-resource-key2",
                                                List.of(
                                                        new AbcdResourceSegmentKey(
                                                                "abcd-segmentation-key2",
                                                                "abcd-segment-key2"
                                                        )
                                                )
                                        ),
                                        "abcd-unit-key-2"
                                )
                        )
                )
        );
    }


    private static class MockCloudQuotaServiceClient implements CloudQuotaServiceClient {

        @Override
        public @NotNull List<PQ.@NotNull QuotaMetric> getQuotaMetrics(@NotNull String cloudId) {
            return List.of(
                    PQ.QuotaMetric.newBuilder()
                            .setName("service.quota1.unit1")
                            .setLimit(2)
                            .setUsage(0.9)
                            .build(),
                    PQ.QuotaMetric.newBuilder()
                            .setName("service.quota2.unit2")
                            .setLimit(3)
                            .setUsage(1.1)
                            .build(),
                    PQ.QuotaMetric.newBuilder()
                            .setName("service.quota3.unit3")
                            .setLimit(3)
                            .setUsage(1)
                            .build()
            );
        }

        @Override
        public void updateQuotaMetrics(@NotNull String cloudId, @NotNull List<PQ.@NotNull MetricLimit> metricLimits) {
        }

    }

}
