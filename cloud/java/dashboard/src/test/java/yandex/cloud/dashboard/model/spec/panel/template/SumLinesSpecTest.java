package yandex.cloud.dashboard.model.spec.panel.template;

import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertEquals;

/**
 * @author ssytnik
 */
public class SumLinesSpecTest {

    @Test
    public void disabled() {
        validate(new SumLinesSpec(false), false, false, null, null, null);
    }

    @Test
    public void enabled() {
        validate(new SumLinesSpec(true), true, true, List.of(), "group_lines", List.of("sum"));
    }

    @Test
    public void labels() {
        validate(new SumLinesSpec(List.of("a")), true, false, List.of("a"), "group_by_labels", List.of("sum", "a"));
        validate(new SumLinesSpec(List.of("a", "b")), true, false, List.of("a", "b"), "group_by_labels", List.of("sum", "a", "b"));
    }


    private void validate(SumLinesSpec s, boolean enabled, boolean singleLine, List<String> labels, String fn, List<String> params) {
        assertEquals(enabled, s.isEnabled());
        assertEquals(singleLine, s.isSingleLine());
        assertEquals(labels, s.getLabels());
        assertEquals(fn, s.getCallFn());
        assertEquals(params, s.getCallParams());
    }
}
