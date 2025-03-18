package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Duration;
import java.time.Instant;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.calendar.AllowedDate;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProviderException;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarService;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.ScheduleChangeEvent;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.commune.bazinga.scheduler.ActiveUniqueIdentifierConverter;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public class JobScheduleTask extends AbstractOnetimeTask<JobScheduleTaskParameters> {

    private FlowStateService flowStateService;
    private JobWaitingScheduler jobWaitingScheduler;
    private WorkCalendarService workCalendarService;
    private CiDb db;

    private int maxRetryCount;
    private Duration retry;
    private Duration timeout;

    public JobScheduleTask(
            FlowStateService flowStateService,
            JobWaitingScheduler jobWaitingScheduler,
            WorkCalendarService workCalendarService,
            CiDb db,
            int maxRetryCount,
            Duration retry,
            Duration timeout
    ) {
        super(JobScheduleTaskParameters.class);
        this.flowStateService = flowStateService;
        this.jobWaitingScheduler = jobWaitingScheduler;
        this.workCalendarService = workCalendarService;
        this.db = db;
        this.maxRetryCount = maxRetryCount;
        this.retry = retry;
        this.timeout = timeout;
    }

    public JobScheduleTask(JobScheduleTaskParameters parameters) {
        super(parameters);
    }

    public static JobScheduleTask retry(JobScheduleTask jobScheduleTask) {
        JobScheduleTaskParameters parameters = JobScheduleTaskParameters
                .retry((JobScheduleTaskParameters) jobScheduleTask.getParameters());
        return new JobScheduleTask(parameters);
    }

    @Override
    protected void execute(JobScheduleTaskParameters parameters, ExecutionContext context) throws Exception {
        try {
            FlowLaunchId launchId = parameters.getFlowLaunchId();
            String jobId = parameters.getJobId();
            var cfg = db.currentOrReadOnly(() -> {
                var flowLaunch = db.flowLaunch().get(launchId);
                var launch = db.launches().get(flowLaunch.getLaunchId());
                return new Configuration(flowLaunch, launch);
            });
            var status = cfg.getLaunch().getStatus();
            if (status.isTerminal()) {
                log.info("Stop scheduling, launch {} in terminal status: {}",
                        cfg.getLaunch().getLaunchId(), status);
                return; // ---
            }

            FlowLaunchEntity launch = cfg.flowLaunch;
            JobState jobState = launch.getJobState(jobId);

            if (jobState.isWaitingForScheduleChangeType()
                    && !launch.isDisabled()
                    && !jobState.isManualTrigger()
                    && jobState.isEnableJobSchedulerConstraint()
            ) {
                AllowedDate nextAllowedDate;
                WorkCalendarProviderException calendarProviderException = null;

                try {
                    nextAllowedDate = workCalendarService.getNextAllowedDate(jobState.getJobSchedulerConstraint());
                } catch (WorkCalendarProviderException e) {
                    log.error("Failed to getNextAllowedDate: {}", parameters, e);
                    nextAllowedDate = AllowedDate.dateNotFound();
                    calendarProviderException = e;
                }

                log.info("Next allowed date {}: {}", nextAllowedDate, parameters);

                var nowInstant = Instant.now();
                if (nextAllowedDate.getType() == AllowedDate.Type.DATE_NOT_FOUND) {
                    if (parameters.getSchedulerLaunchNumber() <= maxRetryCount) {
                        var nextAttemptDate = nowInstant.plus(retry);
                        log.info("Next retry attempt at {}: {}", nextAttemptDate, parameters);
                        jobWaitingScheduler.retry(this, nextAttemptDate);
                        return;
                    }

                    if (calendarProviderException != null) {
                        // TODO: may be we should retry infinitely
                        log.info("Stop retrying, max retry count reached and calendarProviderException occurred: {}",
                                parameters);
                        flowStateService.recalc(launchId, new JobExecutorFailedEvent(
                                jobId,
                                jobState.getLaunchNumber(),
                                ResourceRefContainer.empty(),
                                calendarProviderException
                        ));
                        return;
                    }
                    log.info("Stop retrying, max retry count reached: {}", parameters);
                    return;
                }

                if (nextAllowedDate.getType() == AllowedDate.Type.DATE_FOUND) {
                    var startJobDate = Objects.requireNonNull(nextAllowedDate.getInstant());
                    if (startJobDate.isAfter(nowInstant)) {
                        if (parameters.getSchedulerLaunchNumber() <= maxRetryCount) {
                            log.info("Next retry attempt at {} is after now ({}): {}", startJobDate, nowInstant,
                                    parameters);
                            jobWaitingScheduler.retry(this, startJobDate);
                            return;
                        }
                        log.info("Stop retrying, max retry count reached: {}", parameters);
                        return;
                    }
                }

                ScheduleChangeEvent scheduleChangeEvent = new ScheduleChangeEvent(jobId);
                flowStateService.recalc(launchId, scheduleChangeEvent);
            }
        } catch (Throwable throwable) {
            log.error("Exception occurred while execution of JobScheduleTask with parameters {}",
                    parameters, throwable);
            throw throwable;
        }
    }

    @Nullable
    @Override
    public Class<? extends ActiveUniqueIdentifierConverter<?, ?>> getActiveUidConverter() {
        return ActiveUniqueIdentifierConverterImpl.class;
    }

    @Override
    public Duration getTimeout() {
        return timeout;
    }

    public static class ActiveUniqueIdentifierConverterImpl
            extends AsIsActiveUniqueIdentifierConverter<JobScheduleTaskParameters> {
        public ActiveUniqueIdentifierConverterImpl() {
            super(JobScheduleTaskParameters.class);
        }
    }

    @lombok.Value
    private static class Configuration {
        @Nonnull
        FlowLaunchEntity flowLaunch;
        @Nonnull
        Launch launch;
    }

}
