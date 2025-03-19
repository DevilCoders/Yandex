package yandex.cloud.dashboard.model.result.dashboard;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.result.panel.RowOrPanel;

import java.util.List;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Dashboard {
    Annotations annotations;
    boolean editable;
    @JsonInclude
    Void gnetId;
    int graphTooltip;
    Long id;
    Long iteration; // millis
    List<Link> links;
    List<RowOrPanel> panels;
    Refresh refresh; // note: default is null
    long schemaVersion;
    String style;
    List<String> tags;
    Templating templating;
    Time time;
    Timepicker timepicker;
    String timezone; // "", "browser", "utc"
    String title;
    String uid;
    long version;
}
