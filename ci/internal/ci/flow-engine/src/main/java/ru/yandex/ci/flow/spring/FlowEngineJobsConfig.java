package ru.yandex.ci.flow.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.flow.engine.definition.DummyJob;

@Configuration
public class FlowEngineJobsConfig {

    @Bean
    public DummyJob dummyJob() {
        return new DummyJob();
    }
}
