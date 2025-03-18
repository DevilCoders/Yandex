package ru.yandex.ci.tms.metric.ci;

import java.util.List;

import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.client.rm.RmClient;
import ru.yandex.ci.client.rm.RmComponent;
import ru.yandex.ci.core.db.CiMainDb;

@Slf4j
public class RmUsageMetricCronTask extends AbstractUsageMetricTask {

    private final RmClient rmClient;

    public RmUsageMetricCronTask(CiMainDb db, RmClient rmClient, CuratorFramework curator) {
        super(db, curator);
        this.rmClient = rmClient;
    }

    @Override
    public void computeMetric(MetricConsumer consumer) {
        List<RmComponent> rmActiveComponents = StreamEx.of(rmClient.getComponents())
                .filter(RmComponent::isActive)
                .toList();

        long total = rmActiveComponents.size();
        long overCi = StreamEx.of(rmActiveComponents).filter(RmComponent::isOverCi).count();
        long overTe = total - overCi;

        log.info("Total rm components {}, over CI {}, over TE {}", total, overCi, overTe);

        consumer.addMetric(CiSystemsUsageMetrics.RM_RELEASES, (double) total);
        consumer.addMetric(CiSystemsUsageMetrics.RM_TOTAL_PROCESSES, (double) total);
        consumer.addMetric(CiSystemsUsageMetrics.RM_RELEASES_OVER_TE, (double) overTe);
        consumer.addMetric(CiSystemsUsageMetrics.RM_RELEASES_OVER_CI, (double)  overCi);
    }
}
