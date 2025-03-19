package yandex.cloud.dashboard.model.spec.panel;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.dashboard.DashboardSpec;
import yandex.cloud.dashboard.model.spec.dashboard.Uids;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.UiQueryVarSpec;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.UiVarSpec;
import yandex.cloud.dashboard.model.spec.generic.LabelsSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;
import yandex.cloud.dashboard.util.ObjectUtils;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import static com.google.common.base.MoreObjects.firstNonNull;
import static java.util.stream.Collectors.toMap;
import static yandex.cloud.util.BetterCollectors.throwingMerger;

/**
 * @author ssytnik
 */
@With
@Value
public class DrillDownSpec implements Spec {
    String uid;
    String subUid;
    List<String> tags;
    String title;
    Map<String, DdUiVarSpec> ui;
    Map<String, DdUiQueryVarSpec> uiQuery;
    String uiRepeat;
    LabelsSpec labels;

    @Override
    public void validate(SpecValidationContext context) {
        resolveUid(context.getRootUid());
    }

    public String resolveUid(String rootUid) {
        return Uids.resolve(uid, rootUid, subUid);
    }

    public Map<String, DdUiVarSpec> getUiSafe() {
        return firstNonNull(ui, Map.of());
    }

    public Map<String, DdUiQueryVarSpec> getUiQuerySafe() {
        return firstNonNull(uiQuery, Map.of());
    }

    public VariablesSpec toVariablesSpec(DashboardSpec dashboardSpec) {
        return new VariablesSpec(
                dashboardSpec.getVariables().getDsSafe(),
                getUiSafe().entrySet().stream().collect(toMap(
                        Entry::getKey, e -> toUiVarSpec(dashboardSpec, e.getValue()), throwingMerger(), LinkedHashMap::new)),
                getUiQuerySafe().entrySet().stream().collect(toMap(
                        Entry::getKey, e -> toUiQueryVarSpec(dashboardSpec, e.getValue()), throwingMerger(), LinkedHashMap::new)),
                dashboardSpec.getRepeatVariablesSafe(),
                null
        );
    }

    private UiVarSpec toUiVarSpec(DashboardSpec dashboardSpec, DdUiVarSpec ddUiVarSpec) {
        return ObjectUtils.mapOrDefault(ddUiVarSpec.refName,
                refName -> Preconditions.checkNotNull(
                        dashboardSpec.getUiVariablesSafe().get(refName), "Ui variable cannot be found: %s", refName),
                ddUiVarSpec.getSpec());
    }

    private UiQueryVarSpec toUiQueryVarSpec(DashboardSpec dashboardSpec, DdUiQueryVarSpec ddUiQueryVarSpec) {
        return ObjectUtils.mapOrDefault(ddUiQueryVarSpec.refName,
                refName -> Preconditions.checkNotNull(
                        dashboardSpec.getUiQueryVariablesSafe().get(refName), "Query variable cannot be found: %s", refName),
                ddUiQueryVarSpec.getSpec());
    }


    @AllArgsConstructor
    @With
    @Value
    public static class DdUiVarSpec implements Spec {
        String refName;
        UiVarSpec spec;

        @JsonCreator
        public DdUiVarSpec(String refName) {
            this(refName, null);
        }

        @JsonCreator
        public DdUiVarSpec(UiVarSpec spec) {
            this(null, spec);
        }
    }

    @AllArgsConstructor
    @With
    @Value
    public static class DdUiQueryVarSpec implements Spec {
        String refName;
        UiQueryVarSpec spec;

        @JsonCreator
        public DdUiQueryVarSpec(String refName) {
            this(refName, null);
        }

        @JsonCreator
        public DdUiQueryVarSpec(UiQueryVarSpec spec) {
            this(null, spec);
        }
    }
}
