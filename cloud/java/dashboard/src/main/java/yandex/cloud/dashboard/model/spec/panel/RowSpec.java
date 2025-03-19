package yandex.cloud.dashboard.model.spec.panel;

import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;
import yandex.cloud.dashboard.model.spec.generic.QueryParamsSpec;

import java.util.List;

import static com.google.common.base.MoreObjects.firstNonNull;

/**
 * @author ssytnik
 */
@With
@Value
public class RowSpec implements Spec {
    String title;

    String repeat; // TODO support multiple repeat variables
    String uiRepeat;
    GraphParamsSpec graphDefaults;
    QueryParamsSpec queryDefaults;
    Boolean collapsed;

    List<DrillDownSpec> drilldowns;

    List<PanelSpec> panels;


    public List<PanelSpec> getPanelsSafe() {
        return firstNonNull(panels, List.of());
    }
}
