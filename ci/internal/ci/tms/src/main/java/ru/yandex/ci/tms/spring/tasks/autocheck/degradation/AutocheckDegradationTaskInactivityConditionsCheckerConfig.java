package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationTaskInactivityConditionsChecker;

@Configuration
public class AutocheckDegradationTaskInactivityConditionsCheckerConfig {
    @Bean
    public AutocheckDegradationTaskInactivityConditionsChecker inactivityConditionsChecker(
            @Value("${ci.inactivityConditionsChecker.manualChangeTimeout}") Duration manualChangeTimeout,
            @Value("${ci.inactivityConditionsChecker.enablePlatformTimeout}") Duration enablePlatformTimeout,
            @Value("${ci.inactivityConditionsChecker.semaphoreIncreaseTimeout}") Duration semaphoreIncreaseTimeout,
            @Value("${ci.inactivityConditionsChecker.robotLogin}") String robotLogin
    ) {
        return new AutocheckDegradationTaskInactivityConditionsChecker(
                manualChangeTimeout,
                enablePlatformTimeout,
                semaphoreIncreaseTimeout,
                robotLogin);
    }
}
