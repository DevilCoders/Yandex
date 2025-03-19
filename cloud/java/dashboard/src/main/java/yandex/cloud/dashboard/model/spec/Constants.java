package yandex.cloud.dashboard.model.spec;

import lombok.experimental.UtilityClass;
import yandex.cloud.dashboard.model.spec.dashboard.VariablesSpec.UiVarSpec;

import java.util.List;

/**
 * @author ssytnik
 */
@UtilityClass
public class Constants {
    // TODO FormatSpec enum?
    public static final String FORMAT_NONE = "none";
    public static final String FORMAT_SHORT = "short";
    public static final String FORMAT_MS = "ms";
    public static final String FORMAT_S = "s";

    public static final String AUTO_DRILLDOWN_MENU = "Auto drilldown";

    public static final String UI_VAR_ALL_VALUE = "$__all";
    public static final String UI_VAR_ALL_TITLE = "All";

    public static final String UI_VAR_RATE_UNIT_NAME = "rateUnit";
    public static final UiVarSpec UI_VAR_RATE_UNIT = new UiVarSpec(
            List.of("60", "1"), List.of("rpm", "rps"), null, false, false);
}
