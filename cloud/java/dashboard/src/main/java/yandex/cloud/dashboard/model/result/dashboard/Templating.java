package yandex.cloud.dashboard.model.result.dashboard;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Templating {
    List<TemplateVariable> list;
}
