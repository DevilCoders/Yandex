package yandex.cloud.dashboard.model.result.panel;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.result.generic.GridPos;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class DashboardList implements Panel {
    long id;
    GridPos gridPos;
    String title;
    String description;
    String type;

    String query;
    int limit;
    List<String> tags;
    boolean recent;
    boolean search;
    boolean starred;
    boolean headings;
    @JsonInclude
    Integer folderId;
}
