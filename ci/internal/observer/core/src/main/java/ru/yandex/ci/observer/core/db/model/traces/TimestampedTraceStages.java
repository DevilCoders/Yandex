package ru.yandex.ci.observer.core.db.model.traces;

import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiFunction;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class TimestampedTraceStages {
    Map<String, Instant> stagesStarts;
    Map<String, Instant> stagesFinishes;

    public TimestampedTraceStages() {
        stagesStarts = new HashMap<>();
        stagesFinishes = new HashMap<>();
    }

    public void putTraceTime(
            Map<String, List<String>> startTraceStageMapping,
            Map<String, List<String>> finishTraceStageMapping,
            String trace, Instant time
    ) {
        putTraceTime(
                startTraceStageMapping,
                (tm1, tm2) -> (tm1.isBefore(tm2) ? tm1 : tm2),
                trace, time, stagesStarts
        );
        putTraceTime(
                finishTraceStageMapping,
                (tm1, tm2) -> (tm1.isAfter(tm2) ? tm1 : tm2),
                trace, time, stagesFinishes
        );
    }

    public void putStageStart(String stage, Instant time) {
        if (stagesStarts.containsKey(stage)) {
            return;
        }

        stagesStarts.put(stage, time);
    }

    public void putStageFinish(String stage, Instant time) {
        if (stagesFinishes.containsKey(stage)) {
            return;
        }

        stagesFinishes.put(stage, time);
    }

    private void putTraceTime(
            Map<String, List<String>> traceStageMapping,
            BiFunction<Instant, Instant, Instant> merge,
            String trace, Instant time,
            Map<String, Instant> stagesTime
    ) {
        if (!traceStageMapping.containsKey(trace)) {
            return;
        }

        traceStageMapping.get(trace).forEach(stage -> stagesTime.merge(stage, time, merge));
    }
}
