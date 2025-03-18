package ru.yandex.ci.tms.task.autocheck.degradation;

import java.time.Duration;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.te.TestenvDegradationManager;
import ru.yandex.ci.flow.engine.runtime.bazinga.AbstractMutexCronTask;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationMonitoringsSource.MonitoringState;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.SchedulePeriodic;

@Slf4j
public class AutocheckDegradationCronTask extends AbstractMutexCronTask {

    // Precommit degradation constants
    private static final List<TestenvDegradationManager.Platform> TE_PLATFORMS_ORDER = List.of(
            TestenvDegradationManager.Platform.GCC_MSVC_MUSL,
            TestenvDegradationManager.Platform.IOS_ANDROID_CYGWIN,
            TestenvDegradationManager.Platform.SANITIZERS
    );

    // Task execution mode
    private final Duration taskInterval;
    private final boolean dryRun;

    // Precommit degradation
    private final Set<TestenvDegradationManager.Platform> precommitsDegradationPlatforms;

    // Postcommit degradation
    private final boolean postcommitsDegradationEnabled;
    private final AutocheckDegradationPostcommitManager postcommitDegradationManager;

    // Clients
    private final AutocheckDegradationMonitoringsSource monitoringsSource;
    private final TestenvDegradationManager teDegradationManager;
    private final AutocheckDegradationNotificationsManager notificationsManager;
    //Inactivity conditions
    private final AutocheckDegradationTaskInactivityConditionsChecker inactivityChecker;


    public AutocheckDegradationCronTask(
            Duration taskInterval,
            boolean dryRun,
            Set<TestenvDegradationManager.Platform> precommitsDegradationPlatforms,
            boolean postcommitsDegradationEnabled,
            AutocheckDegradationPostcommitManager postcommitDegradationManager,
            AutocheckDegradationMonitoringsSource monitoringsSource,
            TestenvDegradationManager teDegradationManager,
            AutocheckDegradationNotificationsManager notificationsManager,
            AutocheckDegradationTaskInactivityConditionsChecker inactivityChecker,
            CuratorFramework curator
    ) {
        super(curator);
        this.taskInterval = taskInterval;
        this.dryRun = dryRun;

        this.precommitsDegradationPlatforms = Set.copyOf(precommitsDegradationPlatforms);

        this.postcommitsDegradationEnabled = postcommitsDegradationEnabled;
        this.postcommitDegradationManager = postcommitDegradationManager;

        this.monitoringsSource = monitoringsSource;
        this.teDegradationManager = teDegradationManager;
        this.notificationsManager = notificationsManager;

        this.inactivityChecker = inactivityChecker;
    }


    @Override
    public Schedule cronExpression() {
        return new SchedulePeriodic(taskInterval.toSeconds(), TimeUnit.SECONDS);
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) {
        manageDegradationTask();
    }

    private void manageDegradationTask() {
        var precommitPlatformStatuses = teDegradationManager.getPrecommitPlatformStatuses();
        /* Иногда Соломон не возвращает нужные данные, поэтому необходимо вызывать
            monitoringsSource.getMonitoringState до того, как мы поменяли какие-либо семафоры или же отправили
            какие-либо нотификации */
        var monitoring = monitoringsSource.getMonitoringState();

        Preconditions.checkState(
                monitoring.getInflightPrecommits() != null, "Inflight precommits is null, monitoring {}", monitoring
        );

        log.info("precommitPlatformStatuses {}", precommitPlatformStatuses);
        log.info("monitoring state: {}", monitoring);

        Optional<RuntimeException> chartsException;
        if (monitoring.getAutomaticDegradation().isDisabled()) {
            log.info("Trigger is blocked");
            notificationsManager.muteJuggler();
            chartsException = notificationsManager.openChartsCommentsAboutBlockedDegradation(
                    "Automatic degradation blocked (inflight precommits < %d)"
                            .formatted(monitoring.getAutomaticDegradation().getLimit())
            );
        } else {
            log.info("Trigger is not blocked");
            notificationsManager.unmuteJuggler();
            chartsException = notificationsManager.closeChartsCommentsAboutBlockedDegradation();
        }

        var preCommitDegradation = isDegradationRequired(monitoring, "pre-commit");
        var postCommitDegradation = isDegradationRequired(monitoring, "post-commit");

        log.info("preCommitDegradation state {}", preCommitDegradation);
        log.info("postCommitDegradation state {}", postCommitDegradation);

        if (isAutomaticDegradationDisabledDueToManualActions(precommitPlatformStatuses)) {
            if (preCommitDegradation.isRequired()
                    && precommitPlatformStatuses.isAnyPlatformEnabled(precommitsDegradationPlatforms)) {
                return;
            }
            if (postCommitDegradation.isRequired() && postcommitsDegradationEnabled
                    && postcommitDegradationManager.isAnyPostcommitsEnabled()) {
                return;
            }
        }

        if (postCommitDegradation.isRequired()) {
            if (postcommitsDegradationEnabled && postcommitDegradationManager.isAnyPostcommitsEnabled()) {
                log.info(postCommitDegradation.getReason());
                postcommitDegradationManager.changeSemaphoresInDegradationMode();
                log.info("Postcommit degradation enabled");
            }
        }

        if (preCommitDegradation.isRequired()) {
            if (precommitPlatformStatuses.isAnyPlatformEnabled(precommitsDegradationPlatforms)) {
                log.info(preCommitDegradation.getReason());
                tryEnablePrecommitDegradation();
            }
        } else if (precommitPlatformStatuses.isAnyPlatformDisabled(precommitsDegradationPlatforms)) {
            tryDisablePrecommitDegradation(precommitPlatformStatuses);
        }

        monitoringsSource.savePreviousTriggerState(preCommitDegradation.isRequired());

        precommitPlatformStatuses = teDegradationManager.getPrecommitPlatformStatuses();

        if (!postCommitDegradation.isRequired()) {
            var preCommitLastChangeTime = precommitPlatformStatuses.getLastStatusChange().get();

            log.info("preCommitLastChangeTime {}", preCommitLastChangeTime);

            if (postcommitsDegradationEnabled
                    && precommitPlatformStatuses.isAllPlatformsEnabled()
                    && !inactivityChecker.isInEnablePlatformTimeout(preCommitLastChangeTime)
            ) {
                postcommitDegradationManager.changeSemaphoresInNormalMode(monitoring);
            }
        }

        if (chartsException.isPresent()) {
            throw chartsException.get(); // Temporary solution until something better comes up
        }
    }

    private static Degradation isDegradationRequired(MonitoringState monitoring, String name) {
        return monitoring.isTriggered()
                ? Degradation.required("One of major alerts triggered, enabling " + name + " degradation")
                : Degradation.notRequired();
    }

    private boolean isAutomaticDegradationDisabledDueToManualActions(
            TestenvDegradationManager.PlatformsStatuses precommitPlatformStatuses
    ) {
        return inactivityChecker.isAutomaticDegradationDisabledDueToManualActions(precommitPlatformStatuses);
    }

    private void tryEnablePrecommitDegradation() {
        for (TestenvDegradationManager.Platform p : TE_PLATFORMS_ORDER) {
            if (precommitsDegradationPlatforms.contains(p) && !dryRun) {
                teDegradationManager.manageDegradation(p.enableFallbackAction());
            }
        }

        log.info("Precommit degradation enabled");
    }

    private void tryDisablePrecommitDegradation(TestenvDegradationManager.PlatformsStatuses statuses) {
        for (TestenvDegradationManager.Platform p : TE_PLATFORMS_ORDER) {
            if (statuses.isPlatformEnabled(p)) {
                continue;
            }

            if (!precommitsDegradationPlatforms.contains(p) ||
                    inactivityChecker.isInEnablePlatformTimeout(statuses.getLastStatusChange().get())) {
                return;
            }

            if (inactivityChecker.isPlatformDegradationEnabledManually(statuses.getPlatforms().get(p))) {
                log.info(
                        "Degradation for platform %s was enabled manually and cannot be disabled".formatted(p.name())
                );
                return;
            }

            if (!dryRun) {
                teDegradationManager.manageDegradation(p.disableFallbackAction());
            }

            log.info("Degradation for platform " + p.name() + " disabled");

            return;
        }
    }

    @Value
    private static class Degradation {
        boolean required;
        String reason;

        public static Degradation required(@Nonnull String reason) {
            return new Degradation(true, reason);
        }

        public static Degradation notRequired() {
            return new Degradation(false, "");
        }
    }

}
