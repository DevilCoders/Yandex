package yandex.cloud.dashboard.model.result.panel;

import org.assertj.core.api.Assertions;
import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertEquals;

/**
 * @author ssytnik
 */
public class FunctionDescriptorTest {

    @Test
    public void normalCall() {
        assertEquals("diff({})", format("diff"));
        assertEquals("drop_below({}, 0)", format("drop_below", "0"));

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> format("drop_below"))
                .withMessage("Expected 1 parameter(s) but got 0 for 'drop_below': []");
    }

    @Test
    public void rankingCall() {
        assertEquals("top(3, 'avg', {})", format("top", "3", "avg"));
//        assertEquals("top(3, 'avg', {})", format("top_avg", "3"));

        assertEquals("bottom(3, 'avg', {})", format("bottom", "3", "avg"));
//        assertEquals("bottom(3, 'avg', {})", format("bottom_avg", "3"));

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> format("top", "3"))
                .withMessage("Expected 2 parameter(s) but got 1 for 'top': [3]");
    }

    @Test
    public void groupByLabelsCall() {
        assertEquals("avg({}) by (label)", format("group_by_labels", "avg", "label"));

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> format("group_by_labels"))
                .withMessage("Expected 'group_by_labels' params to be: <fn>, label [, label...], but got []");
        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> format("group_by_labels", "avg"))
                .withMessage("Expected 'group_by_labels' params to be: <fn>, label [, label...], but got [avg]");
    }

    @Test
    public void mathCall() {
        assertEquals("{} * 10", format("math", "* 10"));
        assertEquals("{} / 100", format("math", "/ 100"));

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> format("math"))
                .withMessage("Expected one math operation like '/ 100', but got []");
    }

    @Test
    public void unknownFunctionCall() {
        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> format("unknown_function"))
                .withMessage("Unknown function name or alias: 'unknown_function'");
    }

    private String format(String name, String... params) {
        return FunctionDescriptor.get(name).wrapWithCall("{}", List.of(params));
    }
}