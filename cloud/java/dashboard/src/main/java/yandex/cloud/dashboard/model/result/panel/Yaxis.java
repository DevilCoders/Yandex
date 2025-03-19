package yandex.cloud.dashboard.model.result.panel;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Yaxis {
    boolean align;
    @JsonInclude
    Void alignLevel;
}
