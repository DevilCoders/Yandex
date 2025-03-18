package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import java.util.Map;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.core.spring.clients.SandboxClientConfig;
import ru.yandex.ci.tms.data.RangeSelector;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationPostcommitManager;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationStateKeeper;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationTaskInactivityConditionsChecker;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        SandboxClientConfig.class,
        AutocheckDegradationTaskInactivityConditionsCheckerConfig.class,
        AutocheckDegradationCommonConfig.class
})
public class AutocheckDegradationPostcommitManagerConfig {

    @Bean
    public AutocheckDegradationPostcommitManager degradationPostcommitManager(
            @Value("${ci.degradationPostcommitManager.dryRun}") boolean dryRun,
            AutocheckDegradationStateKeeper autocheckDegradationStateKeeper,
            @Value("${ci.degradationPostcommitManager.semaphoreIncreaseCapacity}") int semaphoreIncreaseCapacity,
            @Value("#{${ci.degradationPostcommitManager.autocheckLevelsMapping}}")
                    Map<Integer, Integer> autocheckLevelsMapping,
            @Value("${ci.degradationPostcommitManager.autocheckLevelsExtraValue}") Integer autocheckLevelsExtraValue,
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager,
            AutocheckDegradationTaskInactivityConditionsChecker inactivityConditionsChecker) {

        return new AutocheckDegradationPostcommitManager(
                dryRun,
                semaphoreIncreaseCapacity,
                new RangeSelector<>(autocheckLevelsMapping, autocheckLevelsExtraValue),
                autocheckDegradationStateKeeper,
                sandboxClient,
                bazingaTaskManager,
                inactivityConditionsChecker
        );
    }
}
