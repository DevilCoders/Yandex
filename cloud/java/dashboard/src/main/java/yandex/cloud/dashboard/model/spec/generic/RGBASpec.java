package yandex.cloud.dashboard.model.spec.generic;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;

import java.beans.ConstructorProperties;

import static java.lang.Double.compare;
import static yandex.cloud.dashboard.model.spec.generic.ColorParser.parseRGBA;

/**
 * @author girevoyt
 * @author ssytnik
 */
@With
@Value
public class RGBASpec implements Spec {
    ColorSpec color;
    double alpha;

    @JsonCreator
    public static RGBASpec of(String rgba) {
        return parseRGBA(rgba);
    }

    @JsonCreator
    @ConstructorProperties({"color", "alpha"})
    public RGBASpec(ColorSpec color, double alpha) {
        Preconditions.checkArgument(compare(0, alpha) <= 0 && compare(alpha, 1) <= 0, "Wrong alpha channel: '%s'", alpha);
        this.color = color;
        this.alpha = alpha;
    }
}
