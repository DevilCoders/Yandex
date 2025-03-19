package yandex.cloud.ti.abcd.provider.mdb;

import java.util.List;

import com.google.protobuf.Empty;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.priv.mdb.v2.QuotaServiceGrpc;
import yandex.cloud.priv.quota.PQ;
import yandex.cloud.ti.abcd.provider.CloudQuotaServiceClient;
import yandex.cloud.ti.grpc.IdempotencyKeyInterceptor;

public class MdbCloudQuotaServiceClientImpl implements CloudQuotaServiceClient {

    public static final @NotNull String QUOTA_SERVICE_NAME = QuotaServiceGrpc.SERVICE_NAME;


    private final @NotNull QuotaServiceGrpc.QuotaServiceBlockingStub quotaService;


    public MdbCloudQuotaServiceClientImpl(
            @NotNull QuotaServiceGrpc.QuotaServiceBlockingStub quotaService
    ) {
        this.quotaService = quotaService;
    }


    @Override
    public @NotNull List<PQ.@NotNull QuotaMetric> getQuotaMetrics(
            @NotNull String cloudId
    ) {
        PQ.GetQuotaRequest request = PQ.GetQuotaRequest.newBuilder()
                .setCloudId(cloudId)
                .build();
        PQ.Quota response = quotaService.get(request);
        return response.getMetricsList();
    }

    @Override
    public void updateQuotaMetrics(
            @NotNull String cloudId,
            @NotNull List<PQ.@NotNull MetricLimit> metricLimits
    ) {
        PQ.BatchUpdateQuotaMetricsRequest request = PQ.BatchUpdateQuotaMetricsRequest.newBuilder()
                .setCloudId(cloudId)
                .addAllMetrics(metricLimits)
                .build();
        Empty response = IdempotencyKeyInterceptor.withIdempotencyKey(quotaService)
                .batchUpdateMetric(request);
    }

}
