package ru.yandex.ci.flow.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.engine.runtime.DispatcherJobScheduler;

import static org.mockito.Mockito.mock;

@Configuration
@Import({
        YdbCiTestConfig.class,
        FlowJobSchedulerConfig.class
})
public class FlowJobSchedulerTestConfig {

    @Bean
    public DispatcherJobScheduler jobScheduler(

    ) {
        return mock(DispatcherJobScheduler.class);
    }

}
