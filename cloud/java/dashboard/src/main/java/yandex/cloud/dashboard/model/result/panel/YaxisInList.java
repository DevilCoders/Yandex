package yandex.cloud.dashboard.model.result.panel;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class YaxisInList {
    Integer decimals;
    String format;
    @JsonInclude
    String label;
    int logBase;
    @JsonInclude
    String max;
    @JsonInclude
    String min;
    boolean show;
}
