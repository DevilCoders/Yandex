package ru.yandex.ci.observer.api.statistics.model;

import java.util.Map;

import com.google.common.base.Preconditions;

public class AggregatedStageDuration extends StatisticsItem {
    private static final String SENSOR = "autocheck_stage_duration_seconds";

    public AggregatedStageDuration(Map<String, String> labels, double durationSeconds) {
        super(SENSOR, labels, durationSeconds);
        Preconditions.checkArgument(
                labels.containsKey(SensorLabels.STAGE), "Mandatory sensor label missed: " + SensorLabels.STAGE
        );
    }
}
