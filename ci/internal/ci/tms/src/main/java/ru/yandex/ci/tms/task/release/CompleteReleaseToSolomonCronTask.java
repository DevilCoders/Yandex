package ru.yandex.ci.tms.task.release;

import java.time.Clock;
import java.time.Duration;
import java.util.Collection;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;
import one.util.streamex.StreamEx;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.LaunchTable.LaunchFinishedView;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.tms.client.SolomonClient;
import ru.yandex.ci.tms.client.SolomonMetric;
import ru.yandex.ci.tms.client.SolomonMetrics;
import ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTask.ProcessInfo;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class CompleteReleaseToSolomonCronTask extends CiEngineCronTask {
    private static final int BATCH_SIZE = 1000;

    private final SolomonClient solomonClient;
    private final SolomonClient.RequiredMetricLabels labels;
    private final CiMainDb db;
    private final Clock clock;

    public CompleteReleaseToSolomonCronTask(
            @Nullable CuratorFramework curator,
            SolomonClient solomonClient,
            SolomonClient.RequiredMetricLabels labels,
            CiMainDb db,
            Clock clock
    ) {
        super(Duration.ofMinutes(5), Duration.ofMinutes(5), curator);
        this.solomonClient = solomonClient;
        this.labels = labels;
        this.db = db;
        this.clock = clock;
    }

    @Override
    protected void executeImpl(ExecutionContext executionContext) {
        var active = db.scan().run(() -> db.configStates().findAllVisible(true));
        log.info("Total active config states: {}", active.size());

        var activeProcessesInfo = ReleaseStatusesToJugglerPushCronTask.transformToProcesses(active);

        var launchesComplete = db.scan().run(() -> db.launches().getAllLatestLaunches());
        log.info("Total complete launches: {}", launchesComplete.size());

        var activeLaunchesByProcessId = StreamEx.of(launchesComplete)
                .groupingBy(LaunchFinishedView::getProcessId);

        var now = clock.instant().toEpochMilli();
        var events = activeProcessesInfo.entrySet().stream()
                .map(e -> generateSolomonEventForProcess(
                        now,
                        e.getKey(),
                        e.getValue(),
                        activeLaunchesByProcessId.getOrDefault(e.getKey().asString(), List.of())
                ))
                .flatMap(Collection::stream)
                .toList();

        log.info("Sending {} events to Solomon", events.size());
        for (var batch : Lists.partition(events, BATCH_SIZE)) {
            solomonClient.push(labels, SolomonMetrics.of(batch));
            log.info("Pushed {} events", batch.size());
        }
    }

    private List<SolomonMetric> generateSolomonEventForProcess(
            long nowMillis,
            CiProcessId processId,
            ProcessInfo processData,
            List<LaunchFinishedView> lastLaunch
    ) {
        var commonLabels = Map.of(
                "sensor", "activity",
                "config", processId.getPath().toString(),
                "id", processId.getSubId(),
                "type", "release",
                "abc", processData.getProjectId()
        );

        return lastLaunch.stream()
                .map(launch -> {
                    var delay = nowMillis - launch.getFinished().toEpochMilli();
                    return SolomonMetric.builder()
                            .labels(commonLabels)
                            .label("status", launch.getStatus().name())
                            .label("flow", launch.getFlowId())
                            .value((double) delay)
                            .build();
                })
                .toList();


    }

}
