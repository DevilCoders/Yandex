package ru.yandex.ci.storage.tests.spring;

import java.time.Clock;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerService;

@Configuration
@Import(CommonTestConfig.class)
public class StorageTestsLogbrokerConfig {

    @Bean
    public TestLogbrokerService logbrokerService(Clock clock) {
        return new TestLogbrokerService(clock);
    }

}
