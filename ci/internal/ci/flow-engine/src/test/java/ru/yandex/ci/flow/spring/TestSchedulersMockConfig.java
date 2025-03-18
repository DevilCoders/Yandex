package ru.yandex.ci.flow.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Primary;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.helpers.TestLaunchAutoReleaseDelegate;

@Configuration
@Import(YdbCiTestConfig.class)
public class TestSchedulersMockConfig {

    @Bean
    @Primary
    public TestLaunchAutoReleaseDelegate testLaunchAutoReleaseDelegate() {
        return new TestLaunchAutoReleaseDelegate();
    }

    @Bean
    @Primary
    public TestJobScheduler testJobScheduler(CiDb db) {
        return new TestJobScheduler(db);
    }

    @Bean
    @Primary
    public TestJobWaitingScheduler testJobWaitingScheduler(CiDb db) {
        return new TestJobWaitingScheduler(db);
    }
}
