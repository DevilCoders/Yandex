package ru.yandex.ci.observer.api.statistics.model;

import java.util.Map;

import com.google.common.base.Preconditions;
import lombok.ToString;

@ToString
public class AggregatedWindowedStatistics extends StatisticsItem {
    private static final String SENSOR = "aggregated_windowed_statistics";

    public AggregatedWindowedStatistics(Map<String, String> labels, long count) {
        super(SENSOR, labels, (double) count);
        Preconditions.checkArgument(
                labels.containsKey(SensorLabels.WINDOW), "Mandatory sensor label missed: " + SensorLabels.WINDOW
        );
    }
}
