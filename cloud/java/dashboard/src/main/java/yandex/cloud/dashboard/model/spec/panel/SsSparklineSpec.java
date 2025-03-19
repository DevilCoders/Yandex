package yandex.cloud.dashboard.model.spec.panel;

import lombok.Builder;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.generic.ColorSpec;
import yandex.cloud.dashboard.model.spec.generic.RGBASpec;
import yandex.cloud.dashboard.util.Mergeable;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author girevoyt
 */
@Builder(toBuilder = true)
@With
@Value
public class SsSparklineSpec implements Mergeable<SsSparklineSpec> {
    public static final SsSparklineSpec DEFAULT = new SsSparklineSpec(
            RGBASpec.of("rgba(87, 148, 242, 0.2)"),
            false,
            ColorSpec.BLUE,
            false
    );

    RGBASpec fillColor;
    Boolean fullHeight;
    ColorSpec lineColor;
    Boolean show;

    @Override
    public SsSparklineSpec merge(SsSparklineSpec lowerPrecedence) {
        return new SsSparklineSpec(
                firstNonNullOrNull(fillColor, lowerPrecedence.fillColor),
                firstNonNullOrNull(fullHeight, lowerPrecedence.fullHeight),
                firstNonNullOrNull(lineColor, lowerPrecedence.lineColor),
                firstNonNullOrNull(show, lowerPrecedence.show)
        );
    }
}
