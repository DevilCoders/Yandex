package yandex.cloud.dashboard.model.spec.panel.template;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.generic.ColorSpec;
import yandex.cloud.dashboard.model.spec.panel.SinglestatSpec;
import yandex.cloud.dashboard.model.spec.panel.SsColoringSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.List;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNullOrNull;

/**
 * @author girevoyt
 */
@With
@Value
public class TrafficLightsTemplate implements SsTemplate {
    Integer lightsCount;
    List<Double> thresholds;
    List<ColorSpec> colors;
    private static List<ColorSpec> defaultColors = List.of(
            ColorSpec.AQUA,
            ColorSpec.GREEN,
            ColorSpec.YELLOW,
            ColorSpec.RED,
            ColorSpec.MAROON);

    @JsonCreator
    public TrafficLightsTemplate(@JsonProperty(value = "lightsCount") int lightsCount,
                                 @JsonProperty(value = "thresholds") List<Double> thresholds,
                                 @JsonProperty(value = "colors") List<ColorSpec> colors) {
        this.lightsCount = lightsCount;
        this.thresholds = thresholds;
        this.colors = colors;
    }

    @Override
    public void validate(SpecValidationContext context) {
        Preconditions.checkNotNull(thresholds, "Thresholds is a required parameter");
        Preconditions.checkNotNull(lightsCount, "LightsCount is a required parameter");
        Preconditions.checkArgument(thresholds.size() == lightsCount - 1,
                "The number of thresholds does not correspond to the number of colors");
        Preconditions.checkArgument(colors == null || colors.size() == lightsCount,
                "The wrong lights count");
        Preconditions.checkArgument(colors == null && lightsCount <= defaultColors.size(),
                "Not enough default colors");
    }

    @Override
    public SinglestatSpec transform(SinglestatSpec source) {
        SsColoringSpec coloringSpec = SsColoringSpec.builder()
                .colorValue(true)
                .colorPostfix(true)
                .colorPrefix(true)
                .colorsList(firstNonNullOrNull(colors, defaultColors.subList(0, lightsCount)))
                .thresholds(thresholds)
                .build();
        return source.withColoring(coloringSpec);
    }
}
