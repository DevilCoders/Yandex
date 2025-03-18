package ru.yandex.ci.engine.spring;

import java.time.Clock;
import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.calendar.CalendarClient;
import ru.yandex.ci.client.taskletv2.TaskletV2Client;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.engine.WorkCalendarProviderImpl;
import ru.yandex.ci.engine.flow.SandboxTaskExecutor;
import ru.yandex.ci.engine.flow.SandboxTaskLauncher;
import ru.yandex.ci.engine.flow.SandboxTaskPollerSettings;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.flow.TaskBadgeService;
import ru.yandex.ci.engine.job.LaunchSandboxTaskExecutor;
import ru.yandex.ci.engine.job.LaunchSandboxTaskletExecutor;
import ru.yandex.ci.engine.job.LaunchTaskletV2Executor;
import ru.yandex.ci.engine.launch.FlowFactory;
import ru.yandex.ci.engine.launch.FlowLaunchService;
import ru.yandex.ci.engine.launch.FlowLaunchServiceImpl;
import ru.yandex.ci.engine.spring.clients.CalendarClientConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProvider;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;

@Configuration
@Import({
        CalendarClientConfig.class,
        TaskletConfig.class,
        FlowFactoryConfig.class
})
public class FlowExecutionConfig {

    @Bean
    public FlowLaunchService flowLaunchService(
            FlowFactory flowFactory,
            CiDb db,
            FlowStateService flowStateService
    ) {
        return new FlowLaunchServiceImpl(flowFactory, db, flowStateService);
    }

    @Bean
    public SandboxTaskExecutor flowService(
            CiDb db,
            SandboxTaskLauncher sandboxTaskLauncher,
            TaskletMetadataService taskletMetadataService,
            SandboxTaskPollerSettings sandboxPollerSettings,
            TaskBadgeService taskBadgeService
    ) {
        return new SandboxTaskExecutor(
                db,
                sandboxTaskLauncher,
                taskletMetadataService,
                sandboxPollerSettings,
                taskBadgeService
        );
    }

    @Bean
    public JobExecutor launchTaskletExecutor(SandboxTaskExecutor sandboxTaskExecutor) {
        return new LaunchSandboxTaskletExecutor(sandboxTaskExecutor);
    }

    @Bean
    public JobExecutor launchTaskletV2Executor(
            TaskletV2Client taskletV2Client,
            SecurityAccessService securityAccessService,
            TaskletV2MetadataService taskletV2MetadataService,
            TaskletContextProcessor taskletContextProcessor,
            CiDb db,
            @Value("${ci.launchTaskletV2Executor.checksInterval}") Duration checksInterval
    ) {
        return new LaunchTaskletV2Executor(
                taskletV2Client,
                securityAccessService,
                taskletV2MetadataService,
                taskletContextProcessor,
                db,
                checksInterval
        );
    }

    @Bean
    public JobExecutor launchSandboxTaskExecutor(SandboxTaskExecutor sandboxTaskExecutor) {
        return new LaunchSandboxTaskExecutor(sandboxTaskExecutor);
    }

    @Bean
    public WorkCalendarProvider workCalendarProvider(CalendarClient calendarClient) {
        return new WorkCalendarProviderImpl(calendarClient);
    }

    @Bean
    public SandboxTaskPollerSettings sandboxPollerSettings(
            Clock clock,
            @Value("${ci.sandboxPollerSettings.minPollingInterval}") Duration minPollingInterval,
            @Value("${ci.sandboxPollerSettings.maxPollingInterval}") Duration maxPollingInterval,
            @Value("${ci.sandboxPollerSettings.maxWaitOnQuotaExceeded}") Duration maxWaitOnQuotaExceeded,
            @Value("${ci.sandboxPollerSettings.notUseAuditEachNRequests}") int notUseAuditEachNRequests,
            @Value("${ci.sandboxPollerSettings.updateTaskOutputEachNRequests}") int updateTaskOutputEachNRequests,
            @Value("${ci.sandboxPollerSettings.maxUnexpectedErrorsInRow}") int maxUnexpectedErrorsInRow
    ) {
        return new SandboxTaskPollerSettings(
                clock,
                minPollingInterval,
                maxPollingInterval,
                maxWaitOnQuotaExceeded,
                notUseAuditEachNRequests,
                updateTaskOutputEachNRequests,
                maxUnexpectedErrorsInRow
        );
    }
}
