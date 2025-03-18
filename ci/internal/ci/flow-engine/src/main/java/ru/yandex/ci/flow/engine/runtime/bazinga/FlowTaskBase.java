package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Duration;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.ci.flow.engine.runtime.TmsTaskId;
import ru.yandex.commune.bazinga.scheduler.ActiveUniqueIdentifierConverter;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public abstract class FlowTaskBase extends AbstractOnetimeTask<FlowTaskParameters> {

    private JobLauncher jobLauncher;
    private Duration timeout;

    protected FlowTaskBase(
            JobLauncher jobLauncher,
            Duration timeout
    ) {
        super(FlowTaskParameters.class);
        this.jobLauncher = jobLauncher;
        this.timeout = timeout;
    }

    protected FlowTaskBase(FlowTaskParameters parameters) {
        super(parameters);
    }

    @Override
    protected void execute(FlowTaskParameters parameters, ExecutionContext context) throws Exception {
        try {
            jobLauncher.launchJob(parameters.toFullJobLaunchId(), TmsTaskId.fromBazingaId(context.getFullJobId()));
        } catch (Throwable throwable) {
            log.error("Exception occurred while execution of FlowTask with parameters {}",
                    parameters, throwable);
            throw throwable;
        }
    }

    @Nullable
    @Override
    public Class<? extends ActiveUniqueIdentifierConverter<?, ?>> getActiveUidConverter() {
        return ActiveUniqueIdentifierConverterImpl.class;
    }

    public static class ActiveUniqueIdentifierConverterImpl
            extends AsIsActiveUniqueIdentifierConverter<FlowTaskParameters> {
        public ActiveUniqueIdentifierConverterImpl() {
            super(FlowTaskParameters.class);
        }
    }

    @Override
    public Duration getTimeout() {
        return timeout;
    }

}
