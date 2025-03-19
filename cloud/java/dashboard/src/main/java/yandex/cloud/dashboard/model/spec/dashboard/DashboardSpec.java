package yandex.cloud.dashboard.model.spec.dashboard;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Constants;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.DsVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.RepeatVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.UiQueryVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.UiVarSpec;
import yandex.cloud.dashboard.model.spec.generic.GraphParamsSpec;
import yandex.cloud.dashboard.model.spec.generic.QueryParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.GraphSpec;
import yandex.cloud.dashboard.model.spec.panel.PanelSpec;
import yandex.cloud.dashboard.model.spec.panel.RowSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.List;
import java.util.Map;
import java.util.Objects;

import static com.google.common.base.MoreObjects.firstNonNull;
import static java.util.stream.Collectors.toList;
import static yandex.cloud.dashboard.model.spec.Constants.AUTO_DRILLDOWN_MENU;
import static yandex.cloud.dashboard.util.ObjectUtils.addToListIf;
import static yandex.cloud.dashboard.util.ObjectUtils.addToMapIf;
import static yandex.cloud.dashboard.util.ObjectUtils.filterAndCast;

/**
 * @author ssytnik
 */
@With
@Value
public class DashboardSpec implements Spec {
    Long folderId;
    Long id;
    String uid;

    TimeSpec time;
    String refresh;
    String title;

    VariablesSpec variables;

    PointerSharing pointerSharing;
    List<LinkSpec> links;
    List<String> tags;

    GraphParamsSpec graphDefaults;
    QueryParamsSpec queryDefaults;

    List<PanelSpec> panels;
    List<RowSpec> rows;

    public enum PointerSharing { // can switch manually with Ctrl+O (Cmd+O)
        none, line, tooltip
    }


    @Override
    public SpecValidationContext newContext(SpecValidationContext context) {
        return context.withRootUid(uid);
    }

    @Override
    public void validate(SpecValidationContext context) {
        Preconditions.checkArgument(panels == null || rows == null,
                "Either panels, or rows of panels may be specified, but not both");
        Uids.validate(uid);
    }


    public List<RowSpec> getRowsSafe() {
        return firstNonNull(rows, List.of());
    }

    public List<PanelSpec> getFlattenedPanelsSafe() {
        return panels != null ? panels : getRowsSafe().stream()
                .flatMap(r -> r.getPanelsSafe().stream())
                .collect(toList());
    }


    public Map<String, DsVarSpec> getDsVariablesSafe() {
        return getVariablesSafe().getDsSafe();
    }

    public Map<String, UiVarSpec> getUiVariablesSafe() {
        return getVariablesSafe().getUiSafe();
    }

    public Map<String, UiQueryVarSpec> getUiQueryVariablesSafe() {
        return getVariablesSafe().getUiQuerySafe();
    }

    public Map<String, RepeatVarSpec> getRepeatVariablesSafe() {
        return getVariablesSafe().getRepeatSafe();
    }

    public Map<String, String> getReplacementVariablesSafe() {
        return getVariablesSafe().getReplacementSafe();
    }

    private VariablesSpec getVariablesSafe() {
        return firstNonNull(variables, VariablesSpec.EMPTY);
    }


    public boolean hasUiRate() {
        return getFlattenedPanelsSafe().stream()
                .flatMap(p -> filterAndCast(p, GraphSpec.class))
                .anyMatch(GraphSpec::hasUiRate);
    }

    public Map<String, DsVarSpec> getAllDsVariablesSafe() {
        return getDsVariablesSafe();
    }

    public Map<String, UiVarSpec> getAllUiVariablesSafe() {
        return addToMapIf(hasUiRate(), getUiVariablesSafe(), Constants.UI_VAR_RATE_UNIT_NAME, Constants.UI_VAR_RATE_UNIT);
    }

    public Map<String, UiQueryVarSpec> getAllUiQueryVariablesSafe() {
        return getUiQueryVariablesSafe();
    }


    private boolean hasDrilldowns() {
        return getRowsSafe().stream()
                .map(RowSpec::getDrilldowns)
                .anyMatch(Objects::nonNull);
    }

    public String getAutoDrilldownTag() {
        return uid.replace('_', '-') + "-auto-drilldown";
    }

    public LinkSpec getAutoDrilldownLink() {
        return new LinkSpec(AUTO_DRILLDOWN_MENU, null, true, List.of(getAutoDrilldownTag()), null, null);
    }

    public List<LinkSpec> getAllLinks() {
        return addToListIf(hasDrilldowns(), getLinks(), getAutoDrilldownLink());
    }
}
