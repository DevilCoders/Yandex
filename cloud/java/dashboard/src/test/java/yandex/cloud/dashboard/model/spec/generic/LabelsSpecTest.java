package yandex.cloud.dashboard.model.spec.generic;

import org.junit.Assert;
import org.junit.Test;

import java.util.Map;

/**
 * @author ssytnik
 */
public class LabelsSpecTest {

    @Test
    public void parse() {
        Assert.assertEquals(Map.of("a", "abc", "b", "*"), new LabelsSpec("a=abc&b=*").getTags());
        Assert.assertEquals(Map.of("a", "abc", "b", "*"), new LabelsSpec("a=abc,b=*").getTags());
        Assert.assertEquals(Map.of("a", "abc", "b", "*"), new LabelsSpec("a=\"abc\", b=\"*\"").getTags());

        Assert.assertEquals(Map.of("a", "$abc", "b", "@def"), new LabelsSpec("a=$abc, b=@def").getTags());
        Assert.assertEquals(Map.of("a-b", "c-d", "e-f", "g-h"), new LabelsSpec("a-b=c-d, e-f=g-h").getTags());
    }

    @Test
    public void containsAllLabels() {
        Assert.assertTrue(new LabelsSpec("a=b").containsAllLabels(new LabelsSpec("a=c")));
        Assert.assertTrue(new LabelsSpec("a=b&d=e").containsAllLabels(new LabelsSpec("a=c")));

        Assert.assertFalse(new LabelsSpec("a=c").containsAllLabels(new LabelsSpec("a=b&d=e")));
        Assert.assertFalse(new LabelsSpec("a=c&d=e").containsAllLabels(new LabelsSpec("d=e&f=g")));
    }

    @Test
    public void trailingComma() {
        Assert.assertEquals(Map.of("w", "x", "y", "z"), new LabelsSpec("w=x, y=z,").getTags());
    }

    @Test
    public void serializedLabels() {
        LabelsSpec spec = new LabelsSpec("a=b1, c=d2");
        Assert.assertEquals("'a'='b1', 'c'='d2'", spec.getSerializedSelector(false));
        Assert.assertEquals("{'a'='b1', 'c'='d2'}", spec.getSerializedSelector(true));
    }

}
