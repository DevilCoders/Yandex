package ru.yandex.ci.flow.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.temporal.spring.TemporalTestConfig;

@Configuration
@Import({
        TestBeansMockConfig.class,
        TemporalTestConfig.class,
        FlowZkTestConfig.class
})
public class FlowEngineTestConfig {
}
