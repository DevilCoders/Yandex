package yandex.cloud.dashboard.model.result.panel;

import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Legend {
    boolean avg;
    boolean current;
    Boolean hideEmpty;
    Boolean hideZero;
    boolean max;
    boolean min;
    Boolean rightSide;
    boolean show;
    boolean total;
    boolean values;
}
