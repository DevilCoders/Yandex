package ru.yandex.ci.flow.engine.definition.job;

import java.util.List;
import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.JobMultiplyConfig;
import ru.yandex.ci.core.config.a.model.JobSkippedByMode;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.taskletv2.TaskletV2ExecutorContext;
import ru.yandex.ci.flow.engine.definition.Position;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.common.CanRunWhen;
import ru.yandex.ci.flow.engine.definition.common.JobSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.definition.common.ManualTriggerPrompt;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;

/**
 * Основная единица флоу, оборачивает исполняемый класс {@link JobExecutor}
 * и определяет его апстримы.
 */
@ToString
public class Job implements AbstractJob<Job> {
    private final String id;
    private final String title;
    private final String description;
    private final JobMultiplyConfig multiply;
    private final boolean manualTrigger;
    private final ManualTriggerPrompt manualTriggerPrompt;
    private final CanRunWhen canRunWhen;
    private final String conditionalRunExpression;
    private final Set<UpstreamLink<Job>> upstreams;
    private final List<Resource> staticResources;
    private final Set<String> tags;
    private final Stage stage;
    private final boolean shouldHoldPreviousStage;
    private final JobAttemptsConfig retry;
    private final Position position;
    private final JobSchedulerConstraintEntity jobSchedulerConstraint;
    private final InternalExecutorContext internal;
    private final TaskletExecutorContext tasklet;
    private final TaskletV2ExecutorContext taskletV2;
    private final SandboxExecutorContext sandboxTask;
    private final RequirementsConfig requirements;
    private final JobType jobType;
    private final RuntimeConfig jobRuntimeConfig;
    private final JobProperties properties;
    private final JobSkippedByMode skippedByMode;

    public Job(JobBuilder builder, Set<UpstreamLink<Job>> upstreams) {
        this.id = builder.getId();
        this.title = builder.getTitle();
        this.description = builder.getDescription();
        this.multiply = builder.getMultiply();
        this.manualTrigger = builder.hasManualTrigger();
        this.manualTriggerPrompt = builder.getManualTriggerPrompt();
        this.canRunWhen = builder.getCanRunWhen();
        this.conditionalRunExpression = builder.getConditionalRunExpression();
        this.upstreams = upstreams;
        this.staticResources = builder.getResources();
        this.tags = builder.getTags();
        this.stage = builder.getStage();
        this.shouldHoldPreviousStage = builder.getShouldHoldPreviousStage();
        this.retry = builder.getRetry();
        this.position = builder.getPosition();
        this.jobSchedulerConstraint = builder.getJobSchedulerConstraint();
        this.internal = builder.getInternal();
        this.tasklet = builder.getTasklet();
        this.taskletV2 = builder.getTaskletV2();
        this.sandboxTask = builder.getSandboxTask();
        this.requirements = builder.getRequirements();
        this.jobRuntimeConfig = builder.getJobRuntimeConfig();
        this.jobType = builder.getJobType();
        this.properties = builder.getProperties();
        this.skippedByMode = builder.getSkippedByMode();
    }


    @Override
    public String getId() {
        return id;
    }

    public String getTitle() {
        return title;
    }

    public String getDescription() {
        return description;
    }

    public JobMultiplyConfig getMultiply() {
        return multiply;
    }

    public boolean hasManualTrigger() {
        return manualTrigger;
    }

    public ManualTriggerPrompt getManualTriggerPrompt() {
        return manualTriggerPrompt;
    }

    @Override
    public Set<UpstreamLink<Job>> getUpstreams() {
        return upstreams;
    }

    /**
     * Получение списка ресурсов
     *
     * @return Ресурсы, которые передаются сразу при объявлении джобы.
     */
    public List<Resource> getStaticResources() {
        return staticResources;
    }

    public Set<String> getTags() {
        return tags;
    }

    public CanRunWhen getCanRunWhen() {
        return canRunWhen;
    }

    public String getConditionalRunExpression() {
        return conditionalRunExpression;
    }

    public Stage getStage() {
        return stage;
    }

    public boolean getShouldHoldPreviousStage() {
        return shouldHoldPreviousStage;
    }

    public JobAttemptsConfig getRetry() {
        return retry;
    }

    public Position getPosition() {
        return position;
    }

    public JobSchedulerConstraintEntity getJobSchedulerConstraint() {
        return jobSchedulerConstraint;
    }

    public JobType getJobType() {
        return jobType;
    }

    public JobProperties getProperties() {
        return properties;
    }

    public JobSkippedByMode getSkippedByMode() {
        return skippedByMode;
    }

    public ExecutorContext getExecutorContext() {
        return new ExecutorContext(internal, tasklet, taskletV2, sandboxTask, requirements, jobRuntimeConfig);
    }

}
