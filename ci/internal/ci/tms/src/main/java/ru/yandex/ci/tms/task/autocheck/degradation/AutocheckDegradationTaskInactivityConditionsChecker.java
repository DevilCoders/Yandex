package ru.yandex.ci.tms.task.autocheck.degradation;

import java.time.Duration;
import java.time.LocalDateTime;
import java.time.ZoneOffset;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.te.TestenvDegradationManager;

public class AutocheckDegradationTaskInactivityConditionsChecker {
    private static final Logger log = LoggerFactory.getLogger(
            AutocheckDegradationTaskInactivityConditionsChecker.class
    );

    private final Duration manualChangeTimeout;
    private final Duration enablePlatformTimeout;
    private final Duration semaphoreIncreaseTimeout;
    private final String robotLogin;

    public AutocheckDegradationTaskInactivityConditionsChecker(
            Duration manualChangeTimeout,
            Duration enablePlatformTimeout,
            Duration semaphoreIncreaseTimeout,
            String robotLogin
    ) {
        this.manualChangeTimeout = manualChangeTimeout;
        this.enablePlatformTimeout = enablePlatformTimeout;
        this.semaphoreIncreaseTimeout = semaphoreIncreaseTimeout;
        this.robotLogin = robotLogin;
    }

    public boolean isInEnablePlatformTimeout(LocalDateTime time) {
        return LocalDateTime.now(ZoneOffset.UTC).minus(enablePlatformTimeout).isBefore(time);
    }

    public boolean isInSemaphoreIncreaseTimeout(LocalDateTime time) {
        return LocalDateTime.now(ZoneOffset.UTC).minus(semaphoreIncreaseTimeout).isBefore(time);
    }

    public boolean isInManualChangeTimeout(LocalDateTime time) {
        return LocalDateTime.now(ZoneOffset.UTC).minus(manualChangeTimeout).isBefore(time);
    }

    public boolean isAutomaticDegradationDisabledDueToManualActions(
            TestenvDegradationManager.PlatformsStatuses precommitPlatformStatuses
    ) {
        if (precommitPlatformStatuses.getPlatforms()
                .values()
                .stream()
                .anyMatch(
                        s -> s.getStatus().equals(TestenvDegradationManager.Status.enabled)
                                && isInManualChangeTimeout(s.getStatusChangeDateTime())
                                && !s.getStatusChangeLogin().equals(robotLogin))
        ) {
            log.info("Degradation was disabled manually in last {}", getManualChangeTimeout());
            return true;
        }

        return false;
    }

    public boolean isPlatformDegradationEnabledManually(TestenvDegradationManager.PlatformStatus status) {
        if (status.getStatus().equals(TestenvDegradationManager.Status.disabled)
                && isNotRobot(status.getStatusChangeLogin())) {

            log.info("Degradation for platform {} was enabled manually by {}",
                    status.getName(), status.getStatusChangeLogin());
            return true;
        }

        return false;
    }

    public boolean isNotRobot(String user) {
        return !robotLogin.equals(user);
    }

    public Duration getManualChangeTimeout() {
        return manualChangeTimeout;
    }

    public Duration getEnablePlatformTimeout() {
        return enablePlatformTimeout;
    }

    public Duration getSemaphoreIncreaseTimeout() {
        return semaphoreIncreaseTimeout;
    }
}
