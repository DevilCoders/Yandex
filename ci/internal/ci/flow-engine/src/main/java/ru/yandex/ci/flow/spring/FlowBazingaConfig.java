package ru.yandex.ci.flow.spring;

import java.time.Clock;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobWaitingSchedulerImpl;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        BazingaCoreConfig.class,
        SourceCodeServiceConfig.class
})
public class FlowBazingaConfig {

    @Bean
    public JobWaitingScheduler jobWaitingSchedulerService(
            BazingaTaskManager bazingaTaskManager,
            Clock clock
    ) {
        return new JobWaitingSchedulerImpl(bazingaTaskManager, clock);
    }
}
