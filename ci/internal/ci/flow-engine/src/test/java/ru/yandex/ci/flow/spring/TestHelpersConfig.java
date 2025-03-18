package ru.yandex.ci.flow.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.FlowProvider;
import ru.yandex.ci.flow.engine.runtime.FlowProviderImpl;
import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestQueries;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTester;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.helpers.TestLaunchAutoReleaseDelegate;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;

@Configuration
@Import({
        YdbCiTestConfig.class,
        TestSchedulersMockConfig.class
})
public class TestHelpersConfig {

    @Bean
    public FlowTestQueries flowTestQueries(CiDb db) {
        return new FlowTestQueries(db);
    }

    @Bean
    public FlowProvider flowProvider() {
        return new FlowProviderImpl();
    }

    @Bean
    public FlowTester flowTester(
            FlowStateService flowStateService,
            TestJobScheduler testJobScheduler,
            TestJobWaitingScheduler testJobWaitingScheduler,
            JobLauncher jobLauncher,
            FlowProvider flowProvider,
            TestLaunchAutoReleaseDelegate launchAutoReleaseDelegate,
            FlowTestQueries flowTestQueries,
            CiDb db
    ) {
        return new FlowTester(
                flowStateService,
                testJobScheduler,
                testJobWaitingScheduler,
                jobLauncher,
                flowProvider,
                launchAutoReleaseDelegate,
                flowTestQueries,
                db);
    }
}
