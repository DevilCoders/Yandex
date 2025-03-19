package yandex.cloud.dashboard.model.spec;

import lombok.Value;
import lombok.With;
import org.junit.Assert;
import org.junit.Test;

import java.util.List;
import java.util.Map;

import static yandex.cloud.dashboard.model.spec.Resolvable.resolve;
import static yandex.cloud.dashboard.util.ObjectUtils.modifyWhileChanging;

/**
 * @author ssytnik
 */
public class ResolvableTest {

    @Test
    public void resolveNull() {
        Assert.assertNull(resolve(null, Map.of()));
    }

    @Test
    public void resolveString() {
//        Assertions.assertThatExceptionOfType(ResolveException.class)
//                .isThrownBy(() -> Resolvable.resolveString("ab @var", Map.of()));
        Assert.assertEquals("ab @var", Resolvable.resolve("ab @var", Map.of()));

        Assert.assertEquals("ab cd cd ef", resolve("ab @var @{var} ef", replacements()));
        Assert.assertEquals("ab cd $var @<var> [[var]] ef", resolve("ab @var $var @<var> [[var]] ef", replacements()));
        Assert.assertEquals("ab cd $var @<var> [[var]] ef", resolve("ab @var $var @<var> [[var]] ef", replacements()));
    }

    @Test
    public void resolveList() {
        Assert.assertEquals(List.of("a", "b", "cd"), resolve(List.of("a", "b", "@var"), replacements()));

        List<String> list = List.of("a", "b", "cd");
        Assert.assertSame(list, resolve(list, replacements()));
    }

    @Test
    public void resolveMap() {
        Assert.assertEquals(Map.of("key", "value", "@var", "cd"),
                resolve(Map.of("key", "value", "@var", "@var"), replacements()));

        Map<String, String> map = Map.of("key", "value");
        Assert.assertSame(map, resolve(map, replacements()));
    }

    @Test
    public void resolveResolvable() {
        R source = new R("abc @var", List.of("abc", "@var"), 10, 20.0, new R("def @var", null, 30, 40.0, null));

        Assert.assertEquals(new R("abc cd", List.of("abc", "cd"), 10, 20.0, new R("def cd", null, 30, 40.0, null)),
                resolve(source, replacements()));
    }

    @Test
    public void resolveMultipassResolvable() {
        Map<String, String> replacements = Map.of("ab", "cd", "cd", "ef");

        Assert.assertEquals(new MR("@{cd}"), resolve(new MR("@{@ab}"), replacements));
        Assert.assertEquals(new MR("ef"), modifyWhileChanging(new MR("@{@ab}"), mr -> resolve(mr, replacements)));
    }

    private Map<String, String> replacements() {
        return Map.of("var", "cd");
    }

    @With
    @Value
    private static class R implements Resolvable {
        String s;
        List list;

        int i;
        Double d;

        R r;
    }

    @With
    @Value
    private static class MR implements Resolvable {
        String s;
    }

}