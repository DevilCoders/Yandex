package yandex.cloud.dashboard.model.result.panel;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.result.dashboard.Link;
import yandex.cloud.dashboard.model.result.dashboard.Options;
import yandex.cloud.dashboard.model.result.generic.GridPos;
import yandex.cloud.dashboard.model.result.generic.RGBColor;

import java.util.List;
import java.util.Map;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Graph implements Panel {
    long id;
    GridPos gridPos;
    String title;
    String description;
    String type;

    Map<String, RGBColor> aliasColors;
    boolean bars;
    int dashLength;
    boolean dashes;
    String datasource;
    Integer decimals;
    int fill;
    Legend legend;
    boolean lines;
    int linewidth;
    List<Link> links;
    String nullPointMode;
    Options options;
    boolean percentage;
    int pointradius;
    boolean points;
    String renderer;
    String repeat;
    String repeatDirection;
    List<SeriesOverride> seriesOverrides;
    int spaceLength;
    boolean stack;
    boolean steppedLine;
    List<Target> targets;
    List<Void> thresholds;
    @JsonInclude
    Void timeFrom;
    List<Void> timeRegions;
    @JsonInclude
    String timeShift;
    Tooltip tooltip;
    Xaxis xaxis;
    List<YaxisInList> yaxes;
    Yaxis yaxis;
}
