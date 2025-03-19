package yandex.cloud.dashboard.model.result.panel;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class FunctionCall {
    String type;
    List<String> params;
}
