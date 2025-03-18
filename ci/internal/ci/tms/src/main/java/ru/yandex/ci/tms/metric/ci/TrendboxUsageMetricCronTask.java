package ru.yandex.ci.tms.metric.ci;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;

import javax.annotation.Nullable;

import one.util.streamex.StreamEx;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.client.trendbox.TrendboxClient;
import ru.yandex.ci.client.trendbox.model.TrendboxScpType;
import ru.yandex.ci.client.trendbox.model.TrendboxWorkflow;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.ydb.service.metric.MetricId;

public class TrendboxUsageMetricCronTask extends AbstractUsageMetricTask {

    private final TrendboxClient trendboxClient;

    public TrendboxUsageMetricCronTask(CiMainDb db, @Nullable CuratorFramework curator,
                                       TrendboxClient trendboxClient) {
        super(db, curator);
        this.trendboxClient = trendboxClient;
    }


    @Override
    public void computeMetric(MetricConsumer consumer) {
        createMetrics(trendboxClient.getWorkflows(), consumer);
    }

    public static List<MetricId> getMetricIds() {
        FakeMetricsConsumer metricsConsumer = new FakeMetricsConsumer();
        createMetrics(List.of(), metricsConsumer);
        return metricsConsumer.getMetricIds();
    }

    private static void createMetrics(List<TrendboxWorkflow> flows, MetricConsumer consumer) {
        var metricIdBuilder = CiSystemUsageMetricIdBuilder.createActive()
                .withTrendbox()
                .withFlowsType();


        forVcs(flows, null, metricIdBuilder.withTotalVcs(), consumer);
        forVcs(flows, TrendboxScpType.ARCADIA, metricIdBuilder.withArcadia(), consumer);
        forVcs(flows, TrendboxScpType.BITBUCKET, metricIdBuilder.withBitbucket(), consumer);
        forVcs(flows, TrendboxScpType.GITHUB, metricIdBuilder.withGithub(), consumer);
    }


    private static void forVcs(List<TrendboxWorkflow> flows, @Nullable TrendboxScpType scpType,
                               CiSystemUsageMetricIdBuilder metricIdBuilder, MetricConsumer consumer) {

        if (scpType != null) {
            flows = StreamEx.of(flows).filter(f -> f.getScpType() == scpType).toList();
        }
        forWindowsDays(flows, 1, metricIdBuilder.withDayWindow(), consumer);
        forWindowsDays(flows, 7, metricIdBuilder.withWeekWindow(), consumer);
        forWindowsDays(flows, 14, metricIdBuilder.with2WeekWindow(), consumer);
        forWindowsDays(flows, 30, metricIdBuilder.withMonthWindow(), consumer);
    }

    private static void forWindowsDays(List<TrendboxWorkflow> flows, int windowDay,
                                       CiSystemUsageMetricIdBuilder metricIdBuilder, MetricConsumer consumer) {

        forWindowsDaysWithStatus(flows, windowDay, true, metricIdBuilder.withSuccessStatus(), consumer);
        forWindowsDaysWithStatus(flows, windowDay, false, metricIdBuilder.withAllStatuses(), consumer);
    }

    private static void forWindowsDaysWithStatus(List<TrendboxWorkflow> flows, int windowDay, boolean onlySuccess,
                                                 CiSystemUsageMetricIdBuilder metricIdBuilder,
                                                 MetricConsumer consumer) {
        Instant lastActualLaunchTime = consumer.now().minus(windowDay, ChronoUnit.DAYS);

        if (onlySuccess) {
            flows = StreamEx.of(flows)
                    .filter(f -> f.getLastSuccessAt() != null)
                    .filter(f -> f.getLastSuccessAt().isAfter(lastActualLaunchTime))
                    .toList();
        } else {
            flows = StreamEx.of(flows).filter(f -> f.getLastLaunchAt().isAfter(lastActualLaunchTime)).toList();
        }
        consumer.addMetric(metricIdBuilder.build(), flows.size());
    }

}
