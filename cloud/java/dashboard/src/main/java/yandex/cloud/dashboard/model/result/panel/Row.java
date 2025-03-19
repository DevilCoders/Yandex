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
public class Row implements RowOrPanel {
    long id;
    GridPos gridPos;
    String title;
    String type;

    boolean collapsed;
    String repeat;
    List<Panel> panels;
}
