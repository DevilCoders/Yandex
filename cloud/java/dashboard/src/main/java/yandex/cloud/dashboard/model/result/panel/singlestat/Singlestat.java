package yandex.cloud.dashboard.model.result.panel.singlestat;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.result.dashboard.Link;
import yandex.cloud.dashboard.model.result.generic.GridPos;
import yandex.cloud.dashboard.model.result.generic.RGBColor;
import yandex.cloud.dashboard.model.result.panel.Panel;
import yandex.cloud.dashboard.model.result.panel.Target;

import java.util.List;

/**
 * @author girevoyt
 */
@AllArgsConstructor
@Data
public class Singlestat implements Panel {
    long id;
    GridPos gridPos;
    String title;
    String description;
    String type;

    @JsonInclude
    String cacheTimeout;
    boolean colorBackground;
    boolean colorPostfix;
    boolean colorPrefix;
    boolean colorValue;
    List<RGBColor> colors;
    String datasource;
    Integer decimals;
    String format;
    Gauge gauge;
    @JsonInclude
    String interval;
    List<Link> links;
    int mappingType;
    List<MappingType> mappingTypes;
    int maxDataPoints;
    String nullPointMode;
    @JsonInclude
    String nullText;
    String postfix;
    String postfixFontSize;
    String prefix;
    String prefixFontSize;
    List<RangeMap> rangeMaps;
    Sparkline sparkline;
    String tableColumn;
    List<Target> targets;
    String thresholds;
    @JsonInclude
    String timeFrom;
    @JsonInclude
    String timeShift;
    String valueFontSize;
    List<ValueMap> valueMaps;
    String valueName;
}
