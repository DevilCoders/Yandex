package ru.yandex.ci.tools.binary_search;

import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;
import ru.yandex.ci.engine.launch.auto.LargePostCommitWriterTask;
import ru.yandex.ci.engine.spring.DiscoveryConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(DiscoveryConfig.class)
public class SendCompletionForSkippedTests extends AbstractSpringBasedApp {

    @Autowired
    LargePostCommitWriterTask task;

    @Autowired
    CiDb db;

    @Override
    protected void run() throws Exception {

        var virtualTypes = List.of(VirtualCiProcessId.VirtualType.values());
        var skippedStatus = List.of(PostponeStatus.SKIPPED, PostponeStatus.FAILURE);
        var processes = db.scan().run(() ->
                db.postponeLaunches().findProcesses(ArcBranch.trunk(), virtualTypes, skippedStatus));
        log.info("Total processes to check: {}", processes.size());

        var executor = Executors.newFixedThreadPool(16);

        for (int i = 0; i < processes.size(); i++) {
            var processId = processes.get(i);
            log.info("[{} out of {}] Processing {}", i, processes.size(), processId);

            var ppLaunches = db.currentOrReadOnly(() ->
                    db.postponeLaunches().findProcessingList(
                            processId,
                            ArcBranch.trunk(),
                            virtualTypes,
                            skippedStatus
                    ));

            log.info("Loaded {} postpone launches", ppLaunches.size());

            var futures = ppLaunches.stream()
                    .map(PostponeLaunch::getId)
                    .map(id -> executor.submit(() -> {
                        task.sendToStorage(id);
                        return null;
                    }))
                    .toList();
            for (var future : futures) {
                future.get();
            }
        }

        executor.shutdown();
        //noinspection ResultOfMethodCallIgnored
        executor.awaitTermination(5, TimeUnit.MINUTES);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
