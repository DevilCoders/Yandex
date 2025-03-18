package ru.yandex.ci.common.temporal.monitoring;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.base.Throwables;
import com.google.common.util.concurrent.AbstractScheduledService;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import io.temporal.activity.ActivityInfo;
import io.temporal.workflow.WorkflowInfo;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.temporal.ydb.TemporalDb;

@Slf4j
public class MetricTemporalMonitoringService extends AbstractScheduledService implements TemporalMonitoringService {

    @VisibleForTesting
    protected static final String RETRY_EXCEEDED_METRIC_NAME = "temporal_workflow_retry_exceeded";

    private final MeterRegistry meterRegistry;
    private final TemporalDb temporalDb;
    private final int activityAttemptCountForWarn;
    private final int activityAttemptCountForCrit;

    private final Map<String, Double> warnRetryExceededWorkflowTypes = new ConcurrentHashMap<>();
    private final Map<String, Double> critRetryExceededWorkflowTypes = new ConcurrentHashMap<>();

    public MetricTemporalMonitoringService(MeterRegistry meterRegistry,
            TemporalDb temporalDb,
            int activityAttemptCountForWarn,
            int activityAttemptCountForCrit
    ) {
        this.meterRegistry = meterRegistry;
        this.temporalDb = temporalDb;
        this.activityAttemptCountForWarn = activityAttemptCountForWarn;
        this.activityAttemptCountForCrit = activityAttemptCountForCrit;
        Preconditions.checkArgument(activityAttemptCountForWarn >= 1);
        Preconditions.checkArgument(activityAttemptCountForCrit > activityAttemptCountForWarn);
    }

    @Override
    public void notifyWorkflowInit(WorkflowInfo info) {
        //TODO
    }

    @Override
    public void notifyWorkflowSuccess(WorkflowInfo info) {
        //TODO
    }

    @Override
    public void notifyWorkflowFailed(WorkflowInfo info, Exception e) {
        //TODO
    }

    @Override
    public void runOneIteration() {
        try {
            updateMetrics();
        } catch (Exception e) {
            log.error("failed to update metrics", e);
            nullAllValues(warnRetryExceededWorkflowTypes);
            nullAllValues(critRetryExceededWorkflowTypes);
        }
    }

    @Override
    protected Scheduler scheduler() {
        return Scheduler.newFixedDelaySchedule(1, 1, TimeUnit.MINUTES);
    }

    @VisibleForTesting
    void updateMetrics() {

        Map<String, Integer> warnTypes = new HashMap<>();
        Map<String, Integer> critTypes = new HashMap<>();

        temporalDb.readOnly().run(
                () -> temporalDb.temporalFailingWorkflow().readTable()
                        .filter(TemporalFailingWorkflowEntity::hasFailingActivity)
                        .forEach(
                                failingWorkflowEntity -> {
                                    String type = failingWorkflowEntity.getWorkflowType();
                                    if (failingWorkflowEntity.getAttemptNumber() >= activityAttemptCountForWarn) {
                                        warnTypes.put(type, warnTypes.getOrDefault(type, 0) + 1);
                                    }
                                    if (failingWorkflowEntity.getAttemptNumber() >= activityAttemptCountForCrit) {
                                        critTypes.put(type, critTypes.getOrDefault(type, 0) + 1);
                                    }
                                }
                        )
        );

        updateMetrics(warnTypes, warnRetryExceededWorkflowTypes, "warn");
        updateMetrics(critTypes, critRetryExceededWorkflowTypes, "crit");
    }

    private void updateMetrics(Map<String, Integer> workflowTypes, Map<String, Double> values, String level) {

        int total = workflowTypes.values().stream().mapToInt(i -> i).sum();
        workflowTypes.put("TOTAL", total);

        for (Map.Entry<String, Integer> workflowTypeEntry : workflowTypes.entrySet()) {
            String workflowType = workflowTypeEntry.getKey();
            if (!values.containsKey(workflowType)) {
                registerMetric(workflowType, values, level);
            }
            double count = workflowTypeEntry.getValue();
            log.info("Workflow type {} has {} failed workflows on severity level {}", workflowType, count, level);
            values.put(workflowType, count);

        }
        nanUnExistingMetrics(workflowTypes.keySet(), values, level);
    }

    private void registerMetric(String workflowType, Map<String, Double> values, String level) {
        Gauge.builder(RETRY_EXCEEDED_METRIC_NAME, () -> values.get(workflowType))
                .tag("type", workflowType)
                .tag("level", level)
                .register(meterRegistry);
    }

    private void nanUnExistingMetrics(Set<String> knownWorkflowTypes, Map<String, Double> values, String level) {
        for (Map.Entry<String, Double> workflowEntry : values.entrySet()) {
            if (!knownWorkflowTypes.contains(workflowEntry.getKey())) {
                log.info(
                        "Workflow type {} has no more failed workflows on severity level {}",
                        workflowEntry.getKey(), level
                );
                workflowEntry.setValue(Double.NaN);
            }
        }
    }

    private void nullAllValues(Map<String, Double> values) {
        for (Map.Entry<String, Double> workflowEntry : values.entrySet()) {
            workflowEntry.setValue(Double.NaN);
        }
    }

    @Override
    public void notifyActivityFailed(ActivityInfo info, Exception e) {
        int attempt = info.getAttempt();
        if (!isFailingActivity(attempt)) {
            return;
        }
        onFailingActivity(info, attempt, e);
    }

    private void onFailingActivity(ActivityInfo info, int attempt, @Nullable Exception e) {
        log.info(
                "Got failing workflow {}, activity {}, runId {}, on attempt {}.",
                info.getWorkflowId(), info.getActivityId(), info.getRunId(), e
        );

        temporalDb.tx(
                () -> temporalDb.temporalFailingWorkflow().save(toEntity(info, attempt, e))
        );
    }

    private TemporalFailingWorkflowEntity toEntity(ActivityInfo info, int attempt, @Nullable Exception e) {
        var builder = TemporalFailingWorkflowEntity.builder()
                .id(TemporalFailingWorkflowEntity.Id.of(info))
                .workflowType(info.getWorkflowType())
                .activityId(info.getActivityId())
                .runId(info.getRunId())
                .attemptNumber(attempt);

        if (e != null) {
            builder.exception(Throwables.getStackTraceAsString(e));
        }
        return builder.build();
    }

    @Override
    public void notifyActivitySuccess(ActivityInfo info) {
        if (!isFailingActivity(info.getAttempt())) {
            return;
        }
        log.info(
                "Activity {} finished successfully. Removing workflow {} from the list of failing. Run {}, attempt {},",
                info.getActivityId(), info.getWorkflowId(), info.getRunId(), info.getAttempt()
        );
        temporalDb.tx(
                () -> temporalDb.temporalFailingWorkflow().delete(TemporalFailingWorkflowEntity.Id.of(info))
        );
    }

    private boolean isFailingActivity(int attempt) {
        return attempt >= activityAttemptCountForWarn;
    }

    @VisibleForTesting
    public void flush() {
        warnRetryExceededWorkflowTypes.clear();
        critRetryExceededWorkflowTypes.clear();
    }

}
