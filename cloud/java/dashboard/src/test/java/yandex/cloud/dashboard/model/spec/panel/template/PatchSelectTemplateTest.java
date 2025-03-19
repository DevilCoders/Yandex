package yandex.cloud.dashboard.model.spec.panel.template;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.dashboard.model.spec.panel.FunctionParamsSpec;
import yandex.cloud.dashboard.model.spec.panel.template.PatchSelectTemplate.ElementSpec;

import java.util.LinkedHashMap;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static yandex.cloud.dashboard.model.spec.panel.QuerySpec.SelectBuilder.selectBuilder;

/**
 * @author ssytnik
 */
public class PatchSelectTemplateTest {
    @Test
    public void positive() {
        LinkedHashMap<String, FunctionParamsSpec>
                at0 = selectBuilder().addCall("fnPatch", "fnP").addCalls(select()).build(),
                at1 = selectBuilder().addCall("fn1", "p1").addCall("fnPatch", "fnP").addCall("fn2", "p21", "p22").build(),
                at2 = selectBuilder().addCalls(select()).addCall("fnPatch", "fnP").build();

        assertOrderedEquals(at0, new PatchSelectTemplate(new ElementSpec("first", null), null, patchSelect()).patch(select()));
        assertOrderedEquals(at1, new PatchSelectTemplate(null, new ElementSpec("first", null), patchSelect()).patch(select()));

        assertOrderedEquals(at1, new PatchSelectTemplate(new ElementSpec("last", null), null, patchSelect()).patch(select()));
        assertOrderedEquals(at2, new PatchSelectTemplate(null, new ElementSpec("last", null), patchSelect()).patch(select()));

        assertOrderedEquals(at0, new PatchSelectTemplate(new ElementSpec(null, 1), null, patchSelect()).patch(select()));
        assertOrderedEquals(at1, new PatchSelectTemplate(null, new ElementSpec(null, 1), patchSelect()).patch(select()));

        assertOrderedEquals(at1, new PatchSelectTemplate(new ElementSpec(null, 2), null, patchSelect()).patch(select()));
        assertOrderedEquals(at2, new PatchSelectTemplate(null, new ElementSpec(null, 2), patchSelect()).patch(select()));
    }

    @Test
    public void negative() {
        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> new PatchSelectTemplate(null, null, patchSelect()).validate(null))
                .withMessageContaining("Either 'before' or 'after' should be specified");

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> new ElementSpec(null, null).validate(null))
                .withMessageContaining("Either 'name' or 'index' should be specified");

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> new ElementSpec(null, 0).validate(null))
                .withMessageContaining("'index' should be positive");

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> new PatchSelectTemplate(new ElementSpec(null, 3), null, patchSelect()).patch(select()))
                .withMessageContaining("'index' is too large: 3");
    }

    @Test
    public void noSelect() {
        assertOrderedEquals(patchSelect(), new PatchSelectTemplate(new ElementSpec("first"), null, patchSelect()).patch(null));
        assertOrderedEquals(patchSelect(), new PatchSelectTemplate(null, new ElementSpec("last"), patchSelect()).patch(null));
        assertOrderedEquals(patchSelect(), new PatchSelectTemplate(null, new ElementSpec(0), patchSelect()).patch(null));

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class)
                .isThrownBy(() -> new PatchSelectTemplate(new ElementSpec(1), null, patchSelect()).patch(null));
    }

    private static LinkedHashMap<String, FunctionParamsSpec> select() {
        return selectBuilder().addCall("fn1", "p1").addCall("fn2", "p21", "p22").build();
    }

    private static LinkedHashMap<String, FunctionParamsSpec> patchSelect() {
        return selectBuilder().addCall("fnPatch", "fnP").build();
    }

    private static void assertOrderedEquals(LinkedHashMap<String, FunctionParamsSpec> expected, LinkedHashMap<String, FunctionParamsSpec> actual) {
        assertEquals(List.copyOf(expected.entrySet()), List.copyOf(actual.entrySet()));
    }
}