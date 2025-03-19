package yandex.cloud.dashboard.model.result.panel;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Xaxis {
    @JsonInclude
    Void buckets;
    String mode;
    @JsonInclude
    String name;
    boolean show;
    List<Void> values;
}
