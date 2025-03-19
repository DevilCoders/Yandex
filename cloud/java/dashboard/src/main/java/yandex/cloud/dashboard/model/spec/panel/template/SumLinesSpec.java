package yandex.cloud.dashboard.model.spec.panel.template;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.collect.ImmutableList;
import lombok.Value;
import lombok.With;
import yandex.cloud.dashboard.model.spec.Spec;

import java.util.List;

/**
 * @author ssytnik
 */
@With
@Value
public class SumLinesSpec implements Spec {
    public static final SumLinesSpec SINGLE_LINE = new SumLinesSpec(true);

    /**
     * null => disabled (do not aggregate);
     * empty => single line (group_lines);
     * non-empty => group_by_labels
     */
    List<String> labels;

    @JsonCreator
    public SumLinesSpec(boolean enabled) {
        this(enabled ? List.of() : null);
    }

    @JsonCreator
    public SumLinesSpec(List<String> labels) {
        this.labels = labels;
    }

    boolean isEnabled() {
        return labels != null;
    }

    boolean isSingleLine() {
        return isEnabled() && labels.isEmpty();
    }

    String getCallFn() {
        return !isEnabled() ? null : isSingleLine() ? "group_lines" : "group_by_labels";
    }

    List<String> getCallParams() {
        return !isEnabled() ? null : ImmutableList.<String>builder().add("sum").addAll(labels).build();
    }
}
