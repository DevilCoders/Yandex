package yandex.cloud.dashboard.model.spec.panel;

import lombok.Builder;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.generic.ColorSpec;
import yandex.cloud.dashboard.util.Mergeable;

import java.util.List;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author girevoyt
 */
@Builder(toBuilder = true)
@With
@Value
public class SsColoringSpec implements Mergeable<SsColoringSpec> {
    public static final SsColoringSpec DEFAULT = new SsColoringSpec(
            false,
            false,
            false,
            false,
            List.of(50D, 80D),
            List.of(ColorSpec.GREEN, ColorSpec.YELLOW, ColorSpec.RED));

    Boolean colorBackground;
    Boolean colorValue;
    Boolean colorPrefix;
    Boolean colorPostfix;
    List<Double> thresholds;
    List<ColorSpec> colorsList;

    @Override
    public SsColoringSpec merge(SsColoringSpec lowerPrecedence) {
        return new SsColoringSpec(
                firstNonNullOrNull(colorBackground, lowerPrecedence.colorBackground),
                firstNonNullOrNull(colorValue, lowerPrecedence.colorValue),
                firstNonNullOrNull(colorPrefix, lowerPrecedence.colorPrefix),
                firstNonNullOrNull(colorPostfix, lowerPrecedence.colorPostfix),
                firstNonNullOrNull(thresholds, lowerPrecedence.thresholds),
                firstNonNullOrNull(colorsList, lowerPrecedence.colorsList)
        );
    }
}
