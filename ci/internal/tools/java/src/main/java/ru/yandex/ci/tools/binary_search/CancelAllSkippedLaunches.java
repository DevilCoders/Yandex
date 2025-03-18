package ru.yandex.ci.tools.binary_search;

import java.util.List;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.spring.LaunchConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(LaunchConfig.class)
public class CancelAllSkippedLaunches extends AbstractSpringBasedApp {

    @Autowired
    LaunchService launchService;

    @Autowired
    CiDb db;

    @Override
    protected void run() {
        var virtualTypes = List.of(VirtualCiProcessId.VirtualType.values());
        var skippedStatus = List.of(PostponeStatus.SKIPPED);
        var processes = db.scan().run(() ->
                db.postponeLaunches().findProcesses(ArcBranch.trunk(), virtualTypes, skippedStatus));
        log.info("Total processes to check: {}", processes.size());

        for (var processId : processes) {
            db.currentOrTx(() -> {
                log.info("Processing {}", processId);

                var ppLaunches = db.postponeLaunches().findProcessingList(
                        processId,
                        ArcBranch.trunk(),
                        virtualTypes,
                        skippedStatus
                );
                log.info("Loaded {} postpone launches", ppLaunches.size());

                var launchIds = ppLaunches.stream()
                        .map(PostponeLaunch::toLaunchId)
                        .collect(Collectors.toSet());
                var launches = db.launches().find(launchIds);
                log.info("Loaded {} launches", launches.size());

                for (var launch : launches) {
                    launchService.cancelDelayedOrPostponedLaunchInTx(launch);
                }
            });
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
