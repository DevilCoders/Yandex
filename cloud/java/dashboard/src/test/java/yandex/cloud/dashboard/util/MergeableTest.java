package yandex.cloud.dashboard.util;

import lombok.Value;
import org.junit.Assert;
import org.junit.Test;

import java.util.List;

import static java.util.Arrays.asList;
import static yandex.cloud.dashboard.util.Mergeable.mergeNullable;

/**
 * @author ssytnik
 */
public class MergeableTest {

    @SuppressWarnings("unchecked")
    @Test
    public void mergeListsTest() {
        Assert.assertNull(mergeNullable((List) null, null));
        Assert.assertEquals(asList(M.of("hv")), mergeNullable(asList(M.of("hv")), null));
        Assert.assertEquals(asList(M.of("lv")), mergeNullable(null, asList(M.of("lv"))));
        Assert.assertEquals(asList(M.of("hv1_lv"), M.of("hv2")),
                mergeNullable(asList(M.of("hv1"), M.of("hv2")), asList(M.of("lv"))));
    }


    @Value(staticConstructor = "of")
    public static class M implements Mergeable<M> {
        String s;

        @Override
        public M merge(M lowerPrecedence) {
            return new M(s + "_" + lowerPrecedence.s);
        }
    }
}