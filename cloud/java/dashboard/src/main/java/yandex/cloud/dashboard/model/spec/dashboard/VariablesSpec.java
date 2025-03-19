package yandex.cloud.dashboard.model.spec.dashboard;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.generic.LabelsSpec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.List;
import java.util.Map;

import static com.google.common.base.MoreObjects.firstNonNull;
import static yandex.cloud.dashboard.model.spec.Constants.UI_VAR_ALL_TITLE;
import static yandex.cloud.dashboard.model.spec.Constants.UI_VAR_ALL_VALUE;

/**
 * @author ssytnik
 */
@With
@Value
public class VariablesSpec implements Spec {
    public static final VariablesSpec EMPTY = new VariablesSpec(null, null, null, null, null);

    Map<String, DsVarSpec> ds;
    Map<String, UiVarSpec> ui;
    Map<String, UiQueryVarSpec> uiQuery;
    Map<String, RepeatVarSpec> repeat;
    Map<String, String> replacement;


    public Map<String, DsVarSpec> getDsSafe() {
        return firstNonNull(ds, Map.of());
    }

    public Map<String, UiVarSpec> getUiSafe() {
        return firstNonNull(ui, Map.of());
    }

    public Map<String, UiQueryVarSpec> getUiQuerySafe() {
        return firstNonNull(uiQuery, Map.of());
    }

    public Map<String, RepeatVarSpec> getRepeatSafe() {
        return firstNonNull(repeat, Map.of());
    }

    public Map<String, String> getReplacementSafe() {
        return firstNonNull(replacement, Map.of());
    }


    public interface ExplicitVarSpec extends Spec {
        List<String> getValues();

        List<String> getTitles();

        ConductorSpec getConductor();

        @Override
        default void validate(SpecValidationContext context) {
            baseValidate(this, context);
        }
    }

    private static void baseValidate(ExplicitVarSpec varSpec, SpecValidationContext context) {
        List<String> values = varSpec.getValues();
        List<String> titles = varSpec.getTitles();
        ConductorSpec conductor = varSpec.getConductor();

        Preconditions.checkArgument(values != null || conductor != null,
                "Explicit values and/or conductor group should be specified");

        if (values == null) {
            Preconditions.checkArgument(titles == null,
                    "Titles cannot be specified if there is no explicit values");
        } else {
            Preconditions.checkArgument(!values.isEmpty(),
                    "Explicit values cannot be empty when specified");

            Preconditions.checkArgument(!values.contains(UI_VAR_ALL_VALUE),
                    "'%s' is not allowed as an explicit value at %s values", UI_VAR_ALL_VALUE, values);

            if (titles != null) {
                Preconditions.checkArgument(values.size() == titles.size(),
                        "Values and titles size should match, but got %s and %s", values, titles);

                Preconditions.checkArgument(!titles.contains(UI_VAR_ALL_TITLE),
                        "'%s' is not allowed as an explicit title at %s titles", UI_VAR_ALL_TITLE, titles);
            }
        }
    }

    @With
    @Value
    public static class DsVarSpec implements Spec {
        String type;
        String regex;
        Boolean hidden;
    }

    @With
    @Value
    public static class UiVarSpec implements ExplicitVarSpec {
        List<String> values;
        List<String> titles;
        ConductorSpec conductor;
        Boolean multi;
        Boolean hidden;
    }

    @AllArgsConstructor
    @With
    @Value
    public static class RepeatVarSpec implements ExplicitVarSpec {
        List<String> values;
        List<String> titles;
        Map<String, List<String>> variables;
        ConductorSpec conductor;

        @Override
        public void validate(SpecValidationContext context) {
            baseValidate(this, context);

            Map<String, List<String>> variables = getVariables();
            List<String> values = getValues();
            if (values == null) {
                Preconditions.checkArgument(variables == null,
                        "Variables cannot be specified if there is no explicit values");
            } else if (getConductor() != null) {
                Preconditions.checkArgument(variables == null,
                        "Variables cannot be specified if there is a conductor group specified");
            } else if (variables != null) {
                variables.forEach(
                        (key, varValues) -> Preconditions.checkArgument(
                                values.size() == varValues.size(),
                                "Values and %s size should match, but got %s and %s", key, values, varValues
                        )
                );
            }
        }
    }

    @With
    @Value
    public static class UiQueryVarSpec implements Spec {
        String datasource;
        LabelsSpec labels;
        Boolean inheritLabels;
        String label;
        String regex;
        Boolean multi;
        Boolean hidden;
        String includeAllValue;
    }

}
