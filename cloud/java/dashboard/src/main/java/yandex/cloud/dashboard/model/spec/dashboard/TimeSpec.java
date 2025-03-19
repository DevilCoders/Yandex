package yandex.cloud.dashboard.model.spec.dashboard;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.result.dashboard.Time;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.model.spec.validator.SpecValidationContext;

import java.util.regex.Pattern;

/**
 * https://grafana.com/docs/grafana/latest/dashboards/time-range-controls/#time-units-and-relative-ranges
 * TODO support explicit timestamps
 *
 * @author ssytnik
 */
@With
@Value
@AllArgsConstructor
public class TimeSpec implements Spec {
    public static final TimeSpec DEFAULT = new TimeSpec("3h");
    private static final Pattern INSTANT_SPEC = Pattern.compile("(?=.)(\\d+[smhdwMy])?(/[smhdwMy])?");

    String from;
    String to;

    @JsonCreator
    public TimeSpec(String from) {
        this(from, null);
    }

    @Override
    public void validate(SpecValidationContext context) {
        Preconditions.checkArgument(from != null, "'from' should be set");
        validate(from);
        validate(to);
    }

    private void validate(String s) {
        Preconditions.checkArgument(s == null || INSTANT_SPEC.matcher(s).matches(), "Wrong time instant spec: %s", s);
    }

    public Time asTime() {
        return new Time(serialize(from), serialize(to));
    }

    private static String serialize(String s) {
        return "now" + (s == null ? "" : (s.startsWith("/") ? "" : "-") + s);
    }
}
