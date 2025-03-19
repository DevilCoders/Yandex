package yandex.cloud.dashboard.model.result.panel.singlestat;

import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * @author girevoyt
 */
@AllArgsConstructor
@Data
public class Gauge {
    long maxValue;
    long minValue;
    boolean show;
    boolean thresholdLabels;
    boolean thresholdMarkers;
}
