package ru.yandex.ci.observer.tests.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.observer.tests.logbroker.TestCommonLogbrokerService;
import ru.yandex.ci.test.clock.OverridableClock;

@Configuration
public class TestLogbrokerConfig {
    @Bean
    public TestCommonLogbrokerService logbrokerService() {
        return new TestCommonLogbrokerService(new OverridableClock());
    }
}
