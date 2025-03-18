package ru.yandex.ci.tms.metric.ci;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import org.apache.curator.framework.CuratorFramework;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.ydb.service.metric.Metric;
import ru.yandex.ci.ydb.service.metric.MetricId;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.SchedulePeriodic;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleWithRetry;

public abstract class AbstractUsageMetricTask extends CiEngineCronTask {

    private static final Logger log = LoggerFactory.getLogger(AbstractUsageMetricTask.class);

    private final CiMainDb db;

    protected AbstractUsageMetricTask(CiMainDb db, @Nullable CuratorFramework curator) {
        super(Duration.ofHours(2), Duration.ofMinutes(10), curator);
        this.db = db;
    }

    @Override
    public Schedule cronExpression() {
        return new ScheduleWithRetry(new SchedulePeriodic(2, TimeUnit.HOURS), 10);
    }

    @Override
    public final void executeImpl(ExecutionContext executionContext) throws Exception {
        MetricConsumerImpl consumer = new MetricConsumerImpl();
        computeMetric(consumer);
        db.currentOrTx(() -> {
            for (Metric metric : consumer.metrics) {
                log.info("Updating metric {}({}) with value {}", metric.getId(), metric.getTime(), metric.getValue());
                db.metrics().save(metric);
            }
        });
    }

    public abstract void computeMetric(MetricConsumer consumer) throws Exception;

    public interface MetricConsumer {
        void addMetric(MetricId metricId, double value);

        void addMetric(MetricId metricId, double value, Instant time);

        Instant now();
    }

    private static class MetricConsumerImpl implements MetricConsumer {
        private final List<Metric> metrics = new ArrayList<>();
        private final Instant now = Instant.now();

        @Override
        public void addMetric(MetricId metricId, double value) {
            metrics.add(Metric.of(metricId, now, value));
        }

        @Override
        public void addMetric(MetricId metricId, double value, Instant time) {
            metrics.add(Metric.of(metricId, time, value));
        }

        @Override
        public Instant now() {
            return now;
        }
    }
}
