package yandex.cloud.dashboard.model.result.generic;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class RGBColor {
    int r;
    int g;
    int b;

    @JsonValue
    public String serialize() {
        return String.format("#%02x%02x%02x", r, g, b);
    }
}
