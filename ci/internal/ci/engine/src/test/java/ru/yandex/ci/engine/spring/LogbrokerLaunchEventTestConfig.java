package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.engine.event.NoOpAsyncProducerWithStore;

@Configuration
public class LogbrokerLaunchEventTestConfig {

    @Bean
    public LogbrokerWriter logbrokerLaunchEventWriter() {
        return new NoOpAsyncProducerWithStore();
    }

}
