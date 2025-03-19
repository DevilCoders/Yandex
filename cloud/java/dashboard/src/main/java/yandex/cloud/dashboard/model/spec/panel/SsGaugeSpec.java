package yandex.cloud.dashboard.model.spec.panel;

import lombok.Builder;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.util.Mergeable;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author girevoyt
 */
@Builder(toBuilder = true)
@With
@Value
public class SsGaugeSpec implements Mergeable<SsGaugeSpec> {
    public static final SsGaugeSpec DEFAULT = new SsGaugeSpec(
            100L,
            0L,
            false,
            false,
            true
    );

    Long maxValue;
    Long minValue;
    Boolean show;
    Boolean thresholdLabels;
    Boolean thresholdMarkers;

    @Override
    public SsGaugeSpec merge(SsGaugeSpec lowerPrecedence) {
        return new SsGaugeSpec(
                firstNonNullOrNull(maxValue, lowerPrecedence.maxValue),
                firstNonNullOrNull(minValue, lowerPrecedence.minValue),
                firstNonNullOrNull(show, lowerPrecedence.show),
                firstNonNullOrNull(thresholdLabels, lowerPrecedence.thresholdLabels),
                firstNonNullOrNull(thresholdMarkers, lowerPrecedence.thresholdMarkers));
    }
}
