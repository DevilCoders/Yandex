package yandex.cloud.dashboard.model.spec.generic;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableMap.Builder;
import com.google.common.collect.ImmutableSortedMap;
import lombok.AllArgsConstructor;
import lombok.NonNull;
import lombok.Value;
import lombok.With;
import lombok.extern.log4j.Log4j2;
import yandex.cloud.dashboard.model.spec.Resolvable;
import yandex.cloud.dashboard.model.spec.Spec;
import yandex.cloud.dashboard.util.Mergeable;
import yandex.cloud.util.Strings;

import java.util.HashMap;
import java.util.Map;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

/**
 * Supported formats:
 * <ul>
 * <li><b>url</b> (<tt>a=b&c=d</tt>) – labels can be copied from Solomon graph url, or Grafana dashboard/panel json;</li>
 * <li><b>Solomon 'copy' button</b> (<tt>a="b", c="d"</tt>) – button can be found at Solomon graph labels line;</li>
 * <li><b>custom format</b> (<tt>a=b,c=d</tt>) – like format above, but spaces and/or quotes can be omitted.</li>
 * </ul>
 *
 * @author ssytnik
 */
@AllArgsConstructor
@With
@Value
@Log4j2
public class LabelsSpec implements Spec, Mergeable<LabelsSpec> {
    // FIXME returns unpredictable results if input is wrong
    // TODO(girevoy) "=!" deprecated, in future need add (==|!==|=~|!~)
    private static final Pattern pattern = Pattern.compile("(\\w[-\\w]*)(!=|=!|=\"!|=)(\"?)([^\",&]*)\\3");

    @AllArgsConstructor
    @Value
    public static class LabelExpression implements Resolvable {
        @NonNull
        @With
        String key;
        @NonNull
        String value;
        @NonNull
        @With
        String comparison;

        public LabelExpression withValue(String newValue) {
            if (newValue != null && newValue.startsWith("!")) {
                Preconditions.checkState(comparison.equals("="),
                        "Repeat is incompatible with '!=' and '!value' at the same time");
                return new LabelExpression(key, newValue.replace("!", ""), "!=");
            }
            return new LabelExpression(key, newValue, comparison);
        }
    }

    Map<String, LabelExpression> value;

    // TODO(girevoy) will change when update solomon plugin for grafana
    public Map<String, String> getTags() {
        return value.values().stream()
                .collect(Collectors.toMap(
                        le -> le.getKey(),
                        le -> (le.getComparison().equals("!=") ? "!" : "") + le.getValue()));
    }

    @JsonCreator
    public LabelsSpec(String labels) {
        Builder<String, LabelExpression> b = ImmutableSortedMap.naturalOrder();
        Strings.forEach(labels, pattern,
                m -> {
                    String comparison = m.group(2);
                    if (comparison.equals("=!") || comparison.equals("=\"!")) {
                        comparison = "!=";
                        log.warn("NOTE: record format 'key=!value' is now deprecated. Please use 'key!=value' format.");
                    }
                    b.put(m.group(1), new LabelExpression(m.group(1), m.group(4), comparison));
                });
        value = b.build();
    }

    @Override
    public LabelsSpec merge(LabelsSpec lowerPrecedence) {
        HashMap<String, LabelExpression> map = new HashMap<>(lowerPrecedence.value);
        map.putAll(value);
        return new LabelsSpec(ImmutableSortedMap.copyOf(map));
    }

    public boolean containsLabel(String name) {
        return value.containsKey(name);
    }

    public boolean containsAllLabels(LabelsSpec spec) {
        return spec.value.keySet().stream().allMatch(this::containsLabel);
    }

    public String getSerializedSelector(boolean brackets) {
        String serializedLabels = value.values().stream()
                .map(e -> "'" + e.getKey() + "'" + e.getComparison() + "'" + e.getValue() + "'")
                .collect(Collectors.joining(", "));
        return brackets ? String.format("{%s}", serializedLabels) : serializedLabels;
    }
}
