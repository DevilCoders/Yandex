package ru.yandex.ci.flow.spring.tasks;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.ci.flow.engine.runtime.temporal.FlowJobActivityImpl;
import ru.yandex.ci.flow.spring.FlowServicesConfig;

@Configuration
@Import({
        FlowServicesConfig.class
})
public class FlowTasksTemporalConfig {

    @Bean
    public FlowJobActivityImpl flowJobActivity(JobLauncher jobLauncher) {
        return new FlowJobActivityImpl(jobLauncher);
    }
}
