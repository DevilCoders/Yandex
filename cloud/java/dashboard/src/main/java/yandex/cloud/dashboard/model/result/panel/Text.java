package yandex.cloud.dashboard.model.result.panel;

import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.result.generic.GridPos;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Text implements Panel {
    long id;
    GridPos gridPos;
    String title;
    String description;
    String type;

    String mode;
    String content;
    List<?> links;
}
