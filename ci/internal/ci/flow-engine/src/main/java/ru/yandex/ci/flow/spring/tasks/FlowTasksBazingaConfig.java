package ru.yandex.ci.flow.spring.tasks;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.ci.flow.engine.runtime.bazinga.FlowTask;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobScheduleTask;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.bazinga.PublicFlowTask;
import ru.yandex.ci.flow.engine.runtime.bazinga.StageRecalcTask;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarService;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.spring.FlowServicesConfig;

@Configuration
@Import({
        FlowServicesConfig.class
})
public class FlowTasksBazingaConfig {

    @Bean
    public FlowTask flowTask(
            JobLauncher jobLauncher,
            @Value("${ci.flowTask.timeout}") Duration timeout
    ) {
        return new FlowTask(jobLauncher, timeout);
    }

    @Bean
    public PublicFlowTask publicFlowTask(
            JobLauncher jobLauncher,
            @Value("${ci.publicFlowTask.timeout}") Duration timeout
    ) {
        return new PublicFlowTask(jobLauncher, timeout);
    }

    @Bean
    public StageRecalcTask stageRecalcTask(
            FlowStateService flowStateService,
            @Value("${ci.stageRecalcTask.timeout}") Duration timeout
    ) {
        return new StageRecalcTask(flowStateService, timeout);
    }

    @Bean
    public JobScheduleTask jobScheduleTask(
            FlowStateService flowStateService,
            JobWaitingScheduler jobWaitingScheduler,
            WorkCalendarService workCalendarService,
            CiDb db,
            @Value("${ci.jobScheduleTask.maxRetryCount}") int maxRetryCount,
            @Value("${ci.jobScheduleTask.retry}") Duration retry,
            @Value("${ci.jobScheduleTask.timeout}") Duration timeout
    ) {
        return new JobScheduleTask(
                flowStateService,
                jobWaitingScheduler,
                workCalendarService,
                db,
                maxRetryCount,
                retry,
                timeout
        );
    }
}
