package ru.yandex.ci.tms.spring.tasks;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.engine.spring.TrackerJobs;
import ru.yandex.ci.engine.spring.tasks.EngineTasksConfig;
import ru.yandex.ci.flow.spring.tasks.FlowTasksBazingaConfig;

@Configuration
@Import({
        EngineTasksConfig.class,
        FlowTasksBazingaConfig.class,
        TrackerJobs.class
})
public class FlowEngineTaskConfig {
}
