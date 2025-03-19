package yandex.cloud.dashboard.model.result.panel;

import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class SeriesOverride {
    String alias;
    Integer yaxis;
    Boolean stack;
}
