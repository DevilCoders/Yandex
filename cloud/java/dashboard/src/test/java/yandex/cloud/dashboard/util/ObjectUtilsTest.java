package yandex.cloud.dashboard.util;

import org.junit.Assert;
import org.junit.Test;

import java.util.List;

/**
 * @author ssytnik
 */
public class ObjectUtilsTest {

    @Test
    public void testModifyWhileChanging() {
        Assert.assertEquals(
                (Integer) 10,
                ObjectUtils.<Integer>modifyWhileChanging(0, i -> i == 10 ? i : i + 1));
    }

    @Test
    public void mapListOfSize() {
        Assert.assertEquals(List.of("c", "c", "c", "c"),
                ObjectUtils.mapListOfSize(4, null, s -> s, () -> "c"));

        Assert.assertEquals(List.of("a", "b", "c", "c"),
                ObjectUtils.mapListOfSize(4, List.of("a", "b"), s -> s, () -> "c"));

        Assert.assertEquals(List.of("aa", "bb", "fill", "fill"),
                ObjectUtils.mapListOfSize(4, List.of("a", "b"), s -> s + s, () -> "fill"));

        Assert.assertEquals(List.of("aa", "bb", "cc", "cc"),
                ObjectUtils.mapListOfSize(4, List.of("a", "b", "c", "c"), s -> s + s, () -> "fill"));
    }

    @Test
    public void addToListIf() {
        Assert.assertNull(ObjectUtils.addToListIf(false, null, "foo"));
        Assert.assertNull(ObjectUtils.addToListIf(true, null));
        Assert.assertEquals(List.of(), ObjectUtils.addToListIf(false, List.of(), "foo"));

        Assert.assertEquals(List.of("foo"), ObjectUtils.addToListIf(true, null, "foo"));
        Assert.assertEquals(List.of("foo"), ObjectUtils.addToListIf(true, List.of(), "foo"));
        Assert.assertEquals(List.of("foo", "bar"), ObjectUtils.addToListIf(true, List.of("foo"), "bar"));
    }
}