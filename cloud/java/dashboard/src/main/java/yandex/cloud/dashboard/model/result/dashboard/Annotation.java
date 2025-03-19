package yandex.cloud.dashboard.model.result.dashboard;

import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.model.result.generic.RGBA;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Annotation {
    int builtIn;
    String datasource;
    boolean enable;
    boolean hide;
    RGBA iconColor;
    String name;
    String type;
}
