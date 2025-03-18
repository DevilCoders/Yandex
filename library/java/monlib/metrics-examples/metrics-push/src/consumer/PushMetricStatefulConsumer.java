package ru.yandex.monlib.metrics.example.push.consumer;

import java.util.Map;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.labels.LabelsBuilder;

/**
 * Stateful metric consumer.
 * Can push RATE metrics as DGAUGE based on previous state
 *
 * @author Alexey Trushkin
 */
public class PushMetricStatefulConsumer extends DelegateMetricConsumer {

    /**
     * state from previous push
     */
    private final Map<Labels, Long> previousValues;
    /**
     * current metric labels builder
     */
    private final LabelsBuilder labelsBuilder = new LabelsBuilder(Labels.MAX_LABELS_COUNT);
    /**
     * metrics grid for rate
     */
    private final int gridSeconds;
    /**
     * labels of current metric
     */
    private Labels currentMetricLabels;
    /**
     * switch metric value logic for RATE
     */
    private boolean currentMetricWasRate;

    public PushMetricStatefulConsumer(MetricConsumer target, Map<Labels, Long> state, int gridSeconds) {
        super(target);
        this.gridSeconds = gridSeconds;
        previousValues = state;
    }

    @Override
    public void onLabel(Label label) {
        super.onLabel(label);
        // add metric label to builder
        labelsBuilder.add(label);
    }

    @Override
    public void onLabelsEnd() {
        super.onLabelsEnd();
        // end of labels building for current metric
        currentMetricLabels = labelsBuilder.build();
    }

    @Override
    public void onLong(long tsMillis, long value) {
        if (currentMetricWasRate) {
            // assume first point is 0
            var previousValue = previousValues.getOrDefault(currentMetricLabels, 0L);
            // diff between previous and current value
            double deltaValue = value - previousValue;
            if (deltaValue > 0) {
                // value in second
                double newValue = deltaValue / gridSeconds;
                super.onDouble(tsMillis, newValue);
            }
            // store value for next iteration
            previousValues.put(currentMetricLabels, value);
        } else {
            // wasn't rate, write as usual
            super.onLong(tsMillis, value);
        }
    }

    @Override
    public void onMetricBegin(MetricType type) {
        // start new labels building
        labelsBuilder.clear();

        // change metric type for RATE, because can't push RATE metrics
        currentMetricWasRate = false;
        if (type == MetricType.RATE) {
            currentMetricWasRate = true;
            type = MetricType.DGAUGE;
        }

        if (type == MetricType.HIST_RATE) {
            type = MetricType.HIST;
        }

        target.onMetricBegin(type);
    }
}
