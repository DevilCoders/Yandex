package ru.yandex.ci.tms.task.autocheck.degradation;

import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.model.Semaphore;
import ru.yandex.ci.tms.data.RangeSelector;
import ru.yandex.ci.tms.data.autocheck.SemaphoreId;
import ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task.AutocheckDegradationPostcommitTasksRestartSchedulerTask;
import ru.yandex.commune.bazinga.BazingaTaskManager;

public class AutocheckDegradationPostcommitManager {

    private static final Logger log = LoggerFactory.getLogger(AutocheckDegradationPostcommitManager.class);
    // Postcommit degradation constants
    private static final List<SemaphoreId> POSTCOMMITS_SEMAPHORE_IDS = List.of(
            SemaphoreId.AUTOCHECK,
            SemaphoreId.AUTOCHECK_FOR_BRANCH
    );

    private final AutocheckDegradationStateKeeper stateKeeper;

    private final boolean dryRun;
    private final int semaphoreIncreaseCapacity;

    //Postcommits capacity managing
    private final RangeSelector<Integer> levelsOfAutocheckSemaphore;

    //Clients
    private final SandboxClient sandboxClient;
    private final BazingaTaskManager bazingaTaskManager;

    //Inactivity conditions
    private final AutocheckDegradationTaskInactivityConditionsChecker inactivityChecker;

    public AutocheckDegradationPostcommitManager(
            boolean dryRun,
            int semaphoreIncreaseCapacity,
            RangeSelector<Integer> levelsOfAutocheckSemaphore,
            AutocheckDegradationStateKeeper stateKeeper,
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager,
            AutocheckDegradationTaskInactivityConditionsChecker inactivityChecker) {
        this.dryRun = dryRun;
        this.semaphoreIncreaseCapacity = semaphoreIncreaseCapacity;

        this.levelsOfAutocheckSemaphore = levelsOfAutocheckSemaphore;

        this.stateKeeper = stateKeeper;
        this.sandboxClient = sandboxClient;
        this.bazingaTaskManager = bazingaTaskManager;
        this.inactivityChecker = inactivityChecker;

    }

    void changeSemaphoresInDegradationMode() {
        log.info("Managing postcommit semaphores in degradation mode");
        for (SemaphoreId semId : POSTCOMMITS_SEMAPHORE_IDS) {
            Semaphore sem = sandboxClient.getSemaphore(semId.getSandboxId());

            if (inactivityChecker.isNotRobot(sem.getAuthor())) {
                stateKeeper.setSemaphoreValue(semId, (int) sem.getCapacity());
            }

            if (!dryRun) {
                if (sem.getCapacity() > 0) {
                    sandboxClient.setSemaphoreCapacity(semId.getSandboxId(), 0, "Degradation enabled");
                }

                if (semId == SemaphoreId.AUTOCHECK) {
                    AutocheckDegradationPostcommitTasksRestartSchedulerTask scheduler =
                            new AutocheckDegradationPostcommitTasksRestartSchedulerTask(
                                    new AutocheckDegradationPostcommitTasksRestartSchedulerTask.Parameters(semId));

                    bazingaTaskManager.schedule(scheduler);
                }
            }
        }
    }

    void changeSemaphoresInNormalMode(AutocheckDegradationMonitoringsSource.MonitoringState monitoring) {
        Preconditions.checkState(monitoring.getInflightPrecommits() != null);
        log.info("Managing postcommit semaphores in normal mode");

        List<Semaphore> semaphores = POSTCOMMITS_SEMAPHORE_IDS.stream()
                .map(semId -> sandboxClient.getSemaphore(semId.getSandboxId())).collect(Collectors.toList());

        Optional<Semaphore> lastUpdatedSemaphore = semaphores.stream().max(
                (sem1, sem2) -> sem1.getUpdateTime().isBefore(sem2.getUpdateTime())
                        ? -1
                        : (sem2.getUpdateTime().isBefore(sem1.getUpdateTime()) ? 1 : 0));

        if (lastUpdatedSemaphore.isEmpty()) {
            throw new RuntimeException("Unable to get last updated semaphore");
        }

        if (inactivityChecker.isInSemaphoreIncreaseTimeout(lastUpdatedSemaphore.get().getUpdateTime())) {
            return;
        }

        for (int i = 0; i < POSTCOMMITS_SEMAPHORE_IDS.size(); ++i) {
            Semaphore sem = semaphores.get(i);
            SemaphoreId semId = POSTCOMMITS_SEMAPHORE_IDS.get(i);

            boolean processed;
            if (semId == SemaphoreId.AUTOCHECK) {
                processed = autocheckSemaphoreChange(
                        sem,
                        levelsOfAutocheckSemaphore.selectRange(monitoring.getInflightPrecommits())
                );
            } else {
                int maxPreviousCapacity = stateKeeper.getSemaphoreValue(semId).orElse(0);
                processed = semaphoreBasicIncrease(sem, semId, maxPreviousCapacity);
            }

            if (!processed) {
                continue;
            }

            return;
        }
    }

    private boolean autocheckSemaphoreChange(Semaphore sem, RangeSelector.Range<Integer> maxRecommendedCapacityRange) {
        if (sem.getCapacity() == maxRecommendedCapacityRange.getValue()) {
            return false;
        }

        if (!dryRun) {
            sandboxClient.setSemaphoreCapacity(
                    SemaphoreId.AUTOCHECK.getSandboxId(),
                    Math.min(sem.getCapacity() + semaphoreIncreaseCapacity, maxRecommendedCapacityRange.getValue()),
                    generateComment(maxRecommendedCapacityRange)
            );

            log.info("Semaphore changed " + generateComment(maxRecommendedCapacityRange));
        }

        return true;
    }

    private boolean semaphoreBasicIncrease(Semaphore sem, SemaphoreId semId, int maxPreviousCapacity) {
        if (sem.getCapacity() >= maxPreviousCapacity) {
            return false;
        }

        if (!dryRun) {
            sandboxClient.setSemaphoreCapacity(
                    semId.getSandboxId(),
                    Math.min(sem.getCapacity() + semaphoreIncreaseCapacity, maxPreviousCapacity),
                    "target capacity: " + maxPreviousCapacity
            );
        }

        return true;
    }

    private String generateComment(RangeSelector.Range<Integer> range) {
        String comment = range.getLowerEndpoint() != null
                ? range.getLowerEndpoint() + " <= inflight precommits "
                : "inflight precommits ";
        if (range.getUpperEndpoint() != null) {
            comment += "< " + range.getUpperEndpoint();
        }

        return comment + " -> max recommended capacity: " + range.getValue();
    }

    boolean isAnyPostcommitsEnabled() {
        return POSTCOMMITS_SEMAPHORE_IDS.stream()
                .anyMatch(semId -> sandboxClient.getSemaphore(semId.getSandboxId()).getCapacity() > 0);
    }
}
