package yandex.cloud.dashboard.model.result.panel.singlestat;

import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.result.generic.RGBA;
import yandex.cloud.dashboard.model.result.generic.RGBColor;

/**
 * @author girevoyt
 */
@AllArgsConstructor
@Data
public class Sparkline {
    RGBA fillColor;
    boolean full;
    RGBColor lineColor;
    boolean show;
}
