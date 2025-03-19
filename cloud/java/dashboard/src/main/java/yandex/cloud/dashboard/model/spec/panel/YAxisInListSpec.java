package yandex.cloud.dashboard.model.spec.panel;

import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.util.Mergeable;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author ssytnik
 */
@With
@Value
public class YAxisInListSpec implements Spec, Mergeable<YAxisInListSpec> {
    Integer decimals; // affects Y axes
    String format;
    String label;
    Integer logBase;
    String max;
    String min;

    @Override
    public YAxisInListSpec merge(YAxisInListSpec lowerPrecedence) {
        return new YAxisInListSpec(
                firstNonNullOrNull(decimals, lowerPrecedence.decimals),
                firstNonNullOrNull(format, lowerPrecedence.format),
                firstNonNullOrNull(label, lowerPrecedence.label),
                firstNonNullOrNull(logBase, lowerPrecedence.logBase),
                firstNonNullOrNull(max, lowerPrecedence.max),
                firstNonNullOrNull(min, lowerPrecedence.min));
    }
}
