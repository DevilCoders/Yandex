package yandex.cloud.dashboard.model.result.dashboard;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Refresh {
    String period;

    @JsonValue
    public Object serialize() {
        return period == null ? false : period;
    }
}
