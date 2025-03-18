package ru.yandex.ci.core.spring;

import java.time.Clock;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.AbcServiceStub;

@Configuration
@Import(CommonTestConfig.class)
public class AbcStubConfig {

    @Bean
    public AbcService abcService(Clock clock) {
        return new AbcServiceStub(clock);
    }
}
