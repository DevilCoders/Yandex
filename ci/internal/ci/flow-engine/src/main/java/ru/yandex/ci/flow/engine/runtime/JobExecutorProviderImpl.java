package ru.yandex.ci.flow.engine.runtime;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

@RequiredArgsConstructor
public class JobExecutorProviderImpl implements JobExecutorProvider {

    @Nonnull
    private final SourceCodeService sourceCodeService;
    @Nonnull
    private final JobExecutor taskletExecutor;
    @Nonnull
    private final JobExecutor taskletV2Executor;
    @Nonnull
    private final JobExecutor sandboxTaskExecutor;
    @Nonnull
    private final TaskletContextProcessor taskletContextProcessor;

    @Override
    public JobExecutor createJobExecutor(JobContext jobContext) {
        var jobState = jobContext.getJobState();
        var executorContext = jobState.getExecutorContext();
        var executorType = ExecutorType.selectFor(executorContext);
        return switch (executorType) {
            case TASKLET -> taskletExecutor;
            case TASKLET_V2 -> taskletV2Executor;
            case SANDBOX_TASK -> sandboxTaskExecutor;
            case CLASS -> buildClassExecutor(jobState, jobContext);
            default -> throw new IllegalStateException("unknown executor type: " + executorType);
        };
    }

    private JobExecutor buildClassExecutor(JobState jobState, JobContext jobContext) {
        var jobExecutorObject = sourceCodeService.getJobExecutor(jobState.getExecutorContext());
        var executor = sourceCodeService.resolveJobExecutorBean(jobExecutorObject);
        taskletContextProcessor.resolveResources(jobContext);
        return executor;
    }
}
