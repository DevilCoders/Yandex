package ru.yandex.ci.flow.engine.definition.context.impl;

import java.util.function.Supplier;

import com.google.common.base.Preconditions;

import ru.yandex.ci.flow.engine.definition.context.JobActionsContext;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.context.JobProgressContext;
import ru.yandex.ci.flow.engine.definition.context.JobResourcesContext;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.engine.runtime.JobUtils;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;

public class JobContextImpl implements JobContext {
    private final JobExecutorObject executorObject;
    private final JobProgressService jobProgressService;
    private final int launchNumber;
    private final JobResourcesContext jobResourcesContext;
    private final JobProgressContext jobProgressContext;
    private final JobActionsContext jobActionsContext;

    private FlowLaunchEntity flowLaunch;
    private JobState jobState;
    private Supplier<FlowLaunchContext> flowLaunchContextSupplier;

    protected JobContextImpl(JobContextImplBuilder builder) {
        this.flowLaunch = builder.getFlowLaunch();
        this.jobState = builder.getJobState();
        this.launchNumber = jobState.getLaunches().size();
        Preconditions.checkState(launchNumber > 0,
                "jobState must have launches");

        this.executorObject = builder.getSourceCodeService().getJobExecutor(jobState.getExecutorContext());
        this.jobProgressService = builder.getJobProgressService();
        this.jobProgressContext = new JobProgressContextImpl(this, jobProgressService);

        this.jobActionsContext = new JobActionsContextImpl(this);
        this.jobResourcesContext = new JobResourcesContextImpl(
                this,
                builder.getUpstreamResourcesCollector(),
                builder.getResourceService()
        );
        this.flowLaunchContextSupplier = () -> FlowLaunchContextBuilder.createContext(this);
    }

    public static JobContextImplBuilder.Builder builder() {
        return JobContextImplBuilder.builder();
    }

    @Override
    public FlowLaunchContext createFlowLaunchContext() {
        return flowLaunchContextSupplier.get();
    }

    @Override
    public FlowLaunchEntity getFlowLaunch() {
        return flowLaunch;
    }

    @Override
    public JobState getJobState() {
        return jobState;
    }

    @Override
    public String getFullJobId() {
        return JobUtils.getFullJobId(flowLaunch, jobState);
    }

    @Override
    public JobResourcesContext resources() {
        return jobResourcesContext;
    }

    @Override
    public JobProgressContext progress() {
        return jobProgressContext;
    }

    @Override
    public JobActionsContext actions() {
        return jobActionsContext;
    }

    @Override
    public int getLaunchNumber() {
        return launchNumber;
    }

    public JobExecutorObject getExecutorObject() {
        return executorObject;
    }

    public void updateFlowLaunch(Supplier<FlowLaunchEntity> updater) {
        FlowLaunchEntity launch = updater.get();

        this.flowLaunch = launch;
        this.jobState = launch.getJobState(getJobId());
        this.flowLaunchContextSupplier = () -> FlowLaunchContextBuilder.createContext(this);
    }

}
