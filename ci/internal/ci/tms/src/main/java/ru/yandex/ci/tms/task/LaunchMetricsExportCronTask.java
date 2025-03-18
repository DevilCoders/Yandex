package ru.yandex.ci.tms.task;

import java.time.Clock;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.ProcessStat;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.tms.client.SolomonClient;
import ru.yandex.ci.tms.client.SolomonMetric;
import ru.yandex.ci.tms.client.SolomonMetrics;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public class LaunchMetricsExportCronTask extends CiEngineCronTask {
    private static final int BATCH_SIZE = 1000;

    private final SolomonClient solomonClient;
    private final SolomonClient.RequiredMetricLabels labels;
    private final CiMainDb db;
    private final Clock clock;

    private static final List<Window> WINDOWS = List.of(
            new Window("1h", Duration.ofHours(1)),
            new Window("15m", Duration.ofMinutes(15)),
            new Window("5m", Duration.ofMinutes(5))
    );

    public LaunchMetricsExportCronTask(
            @Nullable CuratorFramework curator,
            SolomonClient solomonClient,
            SolomonClient.RequiredMetricLabels labels,
            CiMainDb db,
            Clock clock
    ) {
        super(Duration.ofMinutes(1), Duration.ofMinutes(10), curator);
        this.solomonClient = solomonClient;
        this.labels = labels;
        this.db = db;
        this.clock = clock;
    }

    @Override
    protected void executeImpl(ExecutionContext executionContext) {
        for (Window window : WINDOWS) {
            extractInWindow(window);
        }
    }

    private void extractInWindow(Window window) {
        log.info("Processing window {}", window.getLabel());
        var since = clock.instant().minus(window.getDuration());

        var metrics = new ArrayList<SolomonMetric>();
        db.currentOrReadOnly(() -> db.launches().findStats(
                since,
                processStat -> metrics.addAll(mapMetrics(window.getLabel(), processStat))
        ));

        var counter = new AtomicInteger();
        for (var metricsBatch : Lists.partition(metrics, BATCH_SIZE)) {
            sendMetrics(metricsBatch, counter);
        }

        log.info("Processed {}: {} metrics", window.getLabel(), counter);
    }

    private void sendMetrics(List<SolomonMetric> metrics, AtomicInteger counter) {
        log.info("Send metrics batch {}", metrics.size());
        var response = solomonClient.push(labels, SolomonMetrics.of(metrics));
        if (response.getStatus() != SolomonClient.PushStatus.OK) {
            throw new RuntimeException(
                    "solomon respond: " + response.getStatus() + " " + response.getErrorMessage());
        }

        log.info("Response: {}", response);
        counter.addAndGet(metrics.size());
    }

    private List<SolomonMetric> mapMetrics(String window, ProcessStat stat) {
        var processId = CiProcessId.ofString(stat.getProcessId());
        var type = switch (processId.getType()) {
            case RELEASE -> "release";
            case FLOW -> "action";
            case SYSTEM -> throw new IllegalStateException("Unsupported type: " + processId.getType());
        };

        var commonLabels = Map.of(
                "sensor", "activity",
                "config", processId.getPath().toString(),
                "id", processId.getSubId(),
                "type", type,
                "abc", stat.getProject(),
                "activity", stat.getActivity().name().toLowerCase(),
                "window", window
        );

        return List.of(
                SolomonMetric.builder()
                        .labels(commonLabels)
                        .label("metric", "total")
                        .value((double) stat.getCount())
                        .build(),
                SolomonMetric.builder()
                        .labels(commonLabels)
                        .label("metric", "with-retries")
                        .value((double) stat.getWithRetries())
                        .build()
        );
    }

    @Value
    private static class Window {
        String label;
        Duration duration;
    }
}
