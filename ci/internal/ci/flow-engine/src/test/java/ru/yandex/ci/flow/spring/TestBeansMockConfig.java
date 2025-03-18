package ru.yandex.ci.flow.spring;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientTestConfig;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletMetadataServiceImpl;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.calendar.DummyCalendarProviderImpl;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProvider;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchUpdateDelegate;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        YdbCiTestConfig.class,
        CommonTestConfig.class,
        TestHelpersConfig.class,
        TestJobsConfig.class,
        SourceCodeServiceConfig.class,
        FlowControlConfig.class,
        TaskletV2ClientTestConfig.class
})
public class TestBeansMockConfig {

    @Bean
    public FlowLaunchUpdateDelegate flowLaunchUpdateDelegate() {
        return FlowLaunchUpdateDelegate.NO_OP;
    }

    @Bean
    public WorkCalendarProvider workCalendarProvider() {
        return new DummyCalendarProviderImpl();
    }

    @Bean
    public BazingaTaskManager bazingaTaskManager() {
        return Mockito.mock(BazingaTaskManager.class);
    }

    @Bean
    public TaskletMetadataService taskletMetadataService() {
        return Mockito.mock(TaskletMetadataServiceImpl.class);
    }

    @Bean
    public JobExecutor launchTaskletExecutor() {
        return Mockito.mock(JobExecutor.class);
    }

    @Bean
    public JobExecutor launchTaskletV2Executor() {
        return Mockito.mock(JobExecutor.class);
    }

    @Bean
    public JobExecutor launchSandboxTaskExecutor() {
        return Mockito.mock(JobExecutor.class);
    }
}
