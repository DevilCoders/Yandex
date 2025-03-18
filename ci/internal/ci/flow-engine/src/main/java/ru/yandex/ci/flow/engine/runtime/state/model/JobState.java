package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.UUID;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Strings;
import lombok.Data;

import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.JobMultiplyConfig;
import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.flow.engine.definition.Position;
import ru.yandex.ci.flow.engine.definition.common.CanRunWhen;
import ru.yandex.ci.flow.engine.definition.common.JobSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.definition.common.ManualTriggerModifications;
import ru.yandex.ci.flow.engine.definition.common.ManualTriggerPrompt;
import ru.yandex.ci.flow.engine.definition.common.SchedulerConstraintModifications;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.definition.stage.StageRefImpl;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Data
public class JobState {
    private String jobId;
    private String title;
    private String description;
    private ExecutorContext executorContext;
    private String jobTemplateId; // Reference to parent multiply/by task

    private ManualTriggerPrompt manualTriggerPrompt;
    private ManualTriggerModifications manualTriggerModifications;

    private CanRunWhen canRunWhen = CanRunWhen.ALL_COMPLETED;
    private String conditionalRunExpression;
    private JobMultiplyConfig multiply;

    // List of resources that should not be auto-resolved from upstreams
    private Set<JobResourceType> skipUpstreamResources;

    private final List<JobLaunch> launches = new ArrayList<>();
    private final Set<UpstreamLink<String>> upstreams = new HashSet<>();
    private final Set<String> tags = new HashSet<>();

    // Количество оставшихся попыток
    private int retries;
    private JobAttemptsConfig retry;

    @Nullable
    private StageRefImpl stage;

    @Nullable
    private JobSchedulerConstraintEntity jobSchedulerConstraint;
    @Nullable
    private SchedulerConstraintModifications schedulerConstraintModifications;

    private Position position;

    private boolean producesResources;
    private boolean manualTrigger;
    private boolean enableJobSchedulerConstraint = true;
    private boolean isVisible = true;
    /**
     * Проверка на устаревание запусков
     * <p>
     * true, когда у джобы были запуски, но они устарели, иначе - false
     */
    private boolean isOutdated;
    private boolean isReadyToRun;
    private boolean disabled;


    // This job was skipped by conditional 'if' expression
    private boolean conditionalSkip;

    @Nullable
    private GracefulDisablingState gracefulDisablingState;

    @Nullable
    private JobType jobType;

    /**
     * Ресурсы, переданные при объявлении флоу.
     */
    private ResourceRefContainer staticResources;

    // Только для обратной совместимости
    @Deprecated
    private UUID sourceCodeEntityId;

    @SuppressWarnings("UnusedVariable")
    @Deprecated
    private boolean adapter;

    @Nullable
    private DelegatedOutputResources delegatedOutputResources;

    public JobState() {
    }

    public JobState(
            @Nonnull Job job,
            @Nonnull Set<UpstreamLink<String>> upstreams,
            @Nonnull ResourceRefContainer staticResources,
            @Nullable DelegatedOutputResources delegatedOutputResources
    ) {
        this.jobId = job.getId();
        this.title = job.getTitle();
        this.description = job.getDescription();
        this.manualTrigger = job.hasManualTrigger();
        this.manualTriggerPrompt = job.getManualTriggerPrompt();
        this.canRunWhen = job.getCanRunWhen();
        this.conditionalRunExpression = job.getConditionalRunExpression();
        nullSafeSet(this.upstreams, upstreams);
        this.staticResources = staticResources;
        this.delegatedOutputResources = delegatedOutputResources;
        nullSafeSet(this.tags, job.getTags());
        this.retries = job.getRetry() == null ? 0 : job.getRetry().getMaxAttempts() - 1;
        this.retry = job.getRetry();
        this.jobSchedulerConstraint = job.getJobSchedulerConstraint();
        this.position = job.getPosition();
        this.executorContext = job.getExecutorContext();
        this.producesResources = true;
        this.stage = job.getStage() != null ? StageRefImpl.fromStageRef(job.getStage()) : null;
        this.multiply = job.getMultiply();
        this.jobType = job.getJobType();
    }

    private static <T> void nullSafeSet(Collection<T> target, @Nullable Collection<T> source) {
        target.clear();
        if (source != null) {
            target.addAll(source);
        }
    }

    public String getTitleOrId() {
        return Strings.isNullOrEmpty(title) ? jobId : title;
    }

    public boolean isManualTrigger() {
        return manualTrigger && multiply == null; // Template job (with multiply/by) should not have manual trigger
    }

    public boolean awaitsManualTrigger() {
        return isManualTrigger() && (isOutdated || launches.isEmpty()) && isReadyToRun;
    }

    public void clearLaunches() {
        this.launches.clear();
    }

    public boolean hasTag(String tag) {
        return tags.contains(tag);
    }

    @Nullable
    public JobLaunch getLastLaunch() {
        if (launches.isEmpty()) {
            return null;
        }

        return launches.get(launches.size() - 1);
    }

    @Nullable
    public JobLaunch getFirstLaunch() {
        if (launches.isEmpty()) {
            return null;
        }

        return launches.get(0);
    }

    @Nullable
    public StatusChangeType getLastStatusChangeType() {
        JobLaunch lastLaunch = this.getLastLaunch();

        if (lastLaunch == null) {
            return null;
        }

        return lastLaunch.getLastStatusChange().getType();
    }

    public boolean isLastStatusChangeTypeFailed() {
        JobLaunch lastLaunch = this.getLastLaunch();

        return lastLaunch != null && lastLaunch.isLastStatusChangeTypeFailed();
    }

    public boolean isWaitingForScheduleChangeType() {
        return StatusChangeType.WAITING_FOR_SCHEDULE.equals(getLastStatusChangeType());
    }

    public boolean isQueuedChangeType() {
        return StatusChangeType.QUEUED.equals(getLastStatusChangeType());
    }

    public void addLaunch(JobLaunch launch) {
        launches.add(launch);
    }

    public int getLaunchNumber() {
        return launches.size();
    }

    public int getNextLaunchNumber() {
        return getLaunchNumber() + 1;
    }

    public JobLaunch getLaunchByNumber(int jobLaunchNumber) {
        return launches.stream()
                .filter(l -> l.getNumber() == jobLaunchNumber)
                .findFirst()
                .orElseThrow(() ->
                        new NoSuchElementException("Job " + jobId + ", launch " + jobLaunchNumber + " not found"));
    }

    public boolean isExecutorSuccessful() {
        if (this.isOutdated() || this.getLastLaunch() == null) {
            return false;
        }

        return this.getLastLaunch().getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.EXECUTOR_SUCCEEDED
                        || s.getType() == StatusChangeType.FORCED_EXECUTOR_SUCCEEDED);
    }

    public boolean isSuccessful() {
        return !this.isOutdated() && StatusChangeType.SUCCESSFUL.equals(this.getLastStatusChangeType());
    }

    /**
     * @return Номер попытки, начиная с 1
     */
    public int getAttempt() {
        return retry == null ? 1 : retry.getMaxAttempts() - retries;
    }

    public boolean isFailed() {
        if (this.isOutdated() || this.getLastLaunch() == null) {
            return false;
        }

        StatusChangeType lastStatusChangeType = this.getLastStatusChangeType();

        return lastStatusChangeType != null && lastStatusChangeType.isFailed();
    }

    public boolean isInProgress() {
        var lastType = getLastStatusChangeType();
        return lastType != null && !lastType.isFinished();
    }

    public void setDisabling(boolean disabling, boolean ignoreUninterruptableStage) {
        this.gracefulDisablingState = GracefulDisablingState.of(disabling, ignoreUninterruptableStage);
    }

    public boolean isDisabling() {
        return gracefulDisablingState != null && gracefulDisablingState.isInProgress();
    }

    public boolean shouldIgnoreUninterruptibleStage() {
        return gracefulDisablingState != null && gracefulDisablingState.isIgnoreUninterruptibleStage();
    }

    public ExecutorContext getExecutorContext() {
        // Backward compatibility
        if (sourceCodeEntityId != null && executorContext.isLegacyInternal()) {
            this.setExecutorContextInternal(InternalExecutorContext.of(sourceCodeEntityId));
        }
        return this.executorContext;
    }

    @VisibleForTesting
    public void setExecutorContextInternal(InternalExecutorContext internal) {
        this.executorContext = ExecutorContext.internal(internal);
    }
}
