package ru.yandex.ci.tms.task.release;

import java.time.Duration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import com.google.common.collect.Lists;
import com.google.common.collect.Streams;
import lombok.Value;
import one.util.streamex.StreamEx;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.client.juggler.JugglerPushClient;
import ru.yandex.ci.client.juggler.model.RawEvent;
import ru.yandex.ci.client.juggler.model.Status;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchTable.LaunchVersionView;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class ReleaseStatusesToJugglerPushCronTask extends CiEngineCronTask {
    private static final int BATCH_SIZE = 256;
    private static final String COMMON_TAG = "ci-release-status";
    private static final String HAS_ACTIVE_LAUNCHES_TAG = "has-active-launch";

    private final JugglerPushClient jugglerPushClient;
    private final CiMainDb db;
    private final UrlService urlService;
    private final String namespace;

    public ReleaseStatusesToJugglerPushCronTask(
            @Nullable CuratorFramework curator,
            JugglerPushClient jugglerPushClient,
            CiMainDb db,
            @Nullable String namespace,
            UrlService urlService
    ) {
        super(Duration.ofMinutes(1), Duration.ofMinutes(1), curator);
        this.jugglerPushClient = jugglerPushClient;
        this.db = db;
        this.namespace = namespace;
        this.urlService = urlService;
    }

    @Override
    protected void executeImpl(ExecutionContext executionContext) {
        var active = db.scan().run(() -> db.configStates().findAllVisible(true));
        log.info("Total active config states: {}", active.size());

        var activeProcessesInfo = transformToProcesses(active);

        var launchesInActiveProcesses = db.scan().run(() -> db.launches().getAllActiveLaunchVersions());
        log.info("Total launches in active processes: {}", launchesInActiveProcesses.size());

        var activeLaunchesByProcessId = StreamEx.of(launchesInActiveProcesses)
                .groupingBy(LaunchVersionView::getProcessId);

        var events = activeProcessesInfo.entrySet().stream()
                .map(e -> generateJugglerEventForProcess(
                        e.getKey(),
                        e.getValue(),
                        activeLaunchesByProcessId.getOrDefault(e.getKey().asString(), List.of())
                ))
                .toList();

        log.info("Sending {} events to Juggler", events.size());
        for (var batch : Lists.partition(events, BATCH_SIZE)) {
            jugglerPushClient.push(batch);
            log.info("Pushed {} events", batch.size());
        }
    }

    private RawEvent generateJugglerEventForProcess(
            CiProcessId processId,
            ProcessInfo processData,
            List<LaunchVersionView> launches
    ) {
        var statusMap = StreamEx.of(launches)
                .groupingBy(LaunchVersionView::getStatus);

        var projectId = processData.getProjectId();
        var jugglerStatus = Status.OK;
        var description = "";

        var failedLaunches = statusMap.getOrDefault(LaunchState.Status.FAILURE, List.of());
        var runningWithErrors = statusMap.getOrDefault(LaunchState.Status.RUNNING_WITH_ERRORS, List.of());
        var waitingForTrigger = statusMap.getOrDefault(LaunchState.Status.WAITING_FOR_MANUAL_TRIGGER, List.of());
        var idle = statusMap.getOrDefault(LaunchState.Status.IDLE, List.of());
        var delayed = statusMap.getOrDefault(LaunchState.Status.DELAYED, List.of());

        Supplier<String> timelineUrlSupplier = () ->
                urlService.toReleaseTimelineUrl(projectId, processId);
        Function<LaunchVersionView, String> getLaunchUrl = (launch) ->
                urlService.toReleaseLaunch(projectId, launch.getLaunchId(), launch.getVersion());

        if (!failedLaunches.isEmpty()) {
            jugglerStatus = Status.CRIT;
            description = oneOrMore(failedLaunches,
                    single -> "Release is failed and not running %s"
                            .formatted(getLaunchUrl.apply(single)),
                    multiple -> "%s releases are failed and not running %s"
                            .formatted(multiple.size(), timelineUrlSupplier.get())
            );
        } else if (!runningWithErrors.isEmpty()) {
            jugglerStatus = Status.CRIT;
            description = oneOrMore(runningWithErrors,
                    single -> "Release is running but has failed jobs %s"
                            .formatted(getLaunchUrl.apply(single)),
                    multiple -> "%s releases are running but have failed jobs %s"
                            .formatted(multiple.size(), timelineUrlSupplier.get())
            );
        } else if (!delayed.isEmpty()) {
            jugglerStatus = Status.WARN;
            description = oneOrMore(delayed,
                    single -> "Release requires token delegation %s"
                            .formatted(getLaunchUrl.apply(single)),
                    multiple -> "%s releases require token delegation %s"
                            .formatted(multiple.size(), timelineUrlSupplier.get())
            );
        } else if (!waitingForTrigger.isEmpty()) {
            jugglerStatus = Status.WARN;
            description = oneOrMore(waitingForTrigger,
                    single -> "Release is waiting for manual trigger %s"
                            .formatted(getLaunchUrl.apply(single)),
                    multiple -> "%s releases are waiting for manual trigger %s"
                            .formatted(multiple.size(), timelineUrlSupplier.get())
            );
        } else if (!idle.isEmpty()) {
            jugglerStatus = Status.WARN;
            description = oneOrMore(idle,
                    single -> "Release is not running and requires manual action %s"
                            .formatted(getLaunchUrl.apply(single)),
                    multiple -> "%s releases are not running and require manual actions %s"
                            .formatted(multiple.size(), timelineUrlSupplier.get())
            );
        } else {
            description = zeroOrMore(launches,
                    () -> "No running releases %s"
                            .formatted(timelineUrlSupplier.get()),
                    single -> "One release is running %s"
                            .formatted(getLaunchUrl.apply(single)),
                    multiple -> "%s releases are running %s"
                            .formatted(multiple.size(), timelineUrlSupplier.get())
            );
        }

        var tags = Streams.concat(
                Stream.of(
                        withNamespace(COMMON_TAG),
                        "project-" + projectId,
                        "path-" + processId.getPath().toString().replace("/", "__"),
                        "release-" + processId.getSubId()
                ),
                launches.isEmpty()
                        ? Stream.empty()
                        : Stream.of(HAS_ACTIVE_LAUNCHES_TAG),
                processData.getTags().stream()
        ).distinct().toList();

        return RawEvent.builder()
                .host(withNamespace("ci-release-status-" + projectId))
                .service(processId.getSubId())
                .instance(processId.getDir())
                .status(jugglerStatus)
                .description(description)
                .tags(tags)
                .build();
    }

    @VisibleForTesting
    public String withNamespace(String value) {
        if (Strings.isNullOrEmpty(namespace)) {
            return value;
        }
        return namespace + "-" + value;
    }

    private static <T> String zeroOrMore(
            List<T> items,
            Supplier<String> zero,
            Function<T, String> single,
            Function<List<T>, String> multiple
    ) {
        if (items.isEmpty()) {
            return zero.get();
        }
        return oneOrMore(items, single, multiple);
    }

    private static <T> String oneOrMore(List<T> items, Function<T, String> single, Function<List<T>, String> multiple) {
        Preconditions.checkArgument(!items.isEmpty());
        if (items.size() == 1) {
            return single.apply(items.get(0));
        }
        return multiple.apply(items);
    }

    static Map<CiProcessId, ProcessInfo> transformToProcesses(List<ConfigState> active) {
        var result = new HashMap<CiProcessId, ProcessInfo>();
        for (ConfigState configState : active) {
            configState.getReleases().stream()
                    .map(r -> CiProcessId.ofRelease(configState.getConfigPath(), r.getReleaseId()))
                    .forEach(processId -> result.put(processId, ProcessInfo.of(processId, configState)));
        }
        return result;
    }

    @Value
    static class ProcessInfo {
        String projectId;
        List<String> tags;

        public static ProcessInfo of(CiProcessId processId, ConfigState configState) {
            var tags = configState.findRelease(processId.getSubId())
                    .map(ReleaseConfigState::getTags)
                    .orElse(List.of()); // релиз может быть удален из конфигурации, однако быть еще активным
            return new ProcessInfo(configState.getProject(), tags);
        }
    }
}
