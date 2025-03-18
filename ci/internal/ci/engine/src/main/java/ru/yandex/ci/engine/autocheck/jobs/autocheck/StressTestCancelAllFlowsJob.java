package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.Objects;
import java.util.UUID;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.launch.FlowLaunchService;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;

@RequiredArgsConstructor
@ExecutorInfo(
        title = "Stop all running stress test flows",
        description = "Internal job for stopping running stress test flows"
)
@Slf4j
public class StressTestCancelAllFlowsJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("d362081c-4cd4-4d20-b52a-351bcc8eb2b0");

    @Nonnull
    private final AutocheckService autocheckService;

    @Nonnull
    private final FlowLaunchService flowLaunchService;

    @Nonnull
    private final FlowLaunchMutexManager flowLaunchMutexManager;

    @Nonnull
    private final CiMainDb db;


    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var launchIds = db.currentOrReadOnly(() ->
                db.launches().getActiveLaunchIds(AutocheckConstants.STRESS_TEST_TRUNK_PRECOMMIT_PROCESS_ID)
                        .stream()
                        .map(LaunchId::fromKey)
                        .toList()
        );

        log.info("Found launches {}", launchIds.size());

        int success = 0;
        for (var launchId : launchIds) {
            try {
                cancelStorageCheck(launchId);
                cancelAutocheckFlow(launchId);
                success++;
            } catch (Exception e) {
                log.info("Failed to cancel storage check for launch {}", launchId, e);
            }
        }

        log.info("Cancelled launches {}/{}", success, launchIds.size());
    }

    private void cancelAutocheckFlow(LaunchId launchId) {
        log.info("Cancelling autocheck flow for {}", launchId);
        flowLaunchMutexManager.acquireAndRun(
                launchId,
                () -> flowLaunchService.cancelFlow(FlowLaunchId.of(launchId))
        );
        log.info("Cancelled autocheck flow for {}", launchId);
    }

    private void cancelStorageCheck(LaunchId launchId) {
        log.info("Cancelling storage check for {}", launchId);
        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        autocheckService.cancelAutocheckPrecommitFlow(
                Objects.requireNonNull(launch.getFlowLaunchId()),
                launch.getVcsInfo()
        );
        log.info("Cancelled storage check for {}", launchId);
    }
}
