package ru.yandex.ci.common.bazinga.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.test.BazingaTaskManagerStub;

@Configuration
public class BazingaCoreStubConfig {

    @Bean
    public BazingaTaskManager bazingaTaskManager() {
        return new BazingaTaskManagerStub();
    }

}
