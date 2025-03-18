package ru.yandex.ci.observer.core.db.model.traces;

import java.util.HashMap;
import java.util.Map;

import javax.annotation.Nonnull;

import com.google.common.annotations.VisibleForTesting;
import lombok.Value;

import ru.yandex.ci.observer.core.utils.StageAggregationUtils;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class DurationStages {
    @Nonnull
    Map<String, StageDuration> stagesDurations;
    boolean supplement;

    public DurationStages() {
        this(false);
    }

    public DurationStages(boolean skipFirstStage) {
        stagesDurations = new HashMap<>();
        for (var stage : StageAggregationUtils.STAGES) {
            stagesDurations.put(stage, StageDuration.EMPTY);
        }

        if (skipFirstStage) {
            stagesDurations.put(StageAggregationUtils.PRE_CREATION_STAGE, new StageDuration(0L, true));
        }

        supplement = false;
    }

    @VisibleForTesting
    public DurationStages(Map<String, Long> stagesDurationSeconds) {
        stagesDurations = new HashMap<>();
        for (var stage : StageAggregationUtils.STAGES) {
            if (stagesDurationSeconds.containsKey(stage)) {
                stagesDurations.put(stage, new StageDuration(stagesDurationSeconds.get(stage), true));
            } else {
                stagesDurations.put(stage, StageDuration.EMPTY);
            }
        }

        supplement = false;
    }

    public void putStageDuration(String stage, StageDuration duration) {
        stagesDurations.put(stage, duration);
    }

    public void putStageDuration(String stage, long durationSeconds) {
        putStageDuration(stage, new StageDuration(durationSeconds, true));
    }

    public StageDuration getStageDuration(String stage) {
        return stagesDurations.getOrDefault(stage, StageDuration.EMPTY);
    }

    @Value
    @Persisted
    public static class StageDuration {
        public static final StageDuration EMPTY = new StageDuration(0, false);

        long seconds;
        boolean completed;
    }
}
