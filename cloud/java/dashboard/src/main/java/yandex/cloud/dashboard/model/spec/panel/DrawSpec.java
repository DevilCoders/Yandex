package yandex.cloud.dashboard.model.spec.panel;

import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.generic.ColorSpec;

/**
 * @author ssytnik
 */
@With
@Value
public class DrawSpec {
    String alias;
    ColorSpec color;
    At at;
    Boolean stack;

    public enum At {
        left, right
    }
}
