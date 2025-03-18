package ru.yandex.ci.flow.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.TestZkConfig;

@Configuration
@Import({
        FlowZkConfig.class,
        TestZkConfig.class
})
public class FlowZkTestConfig {
}
