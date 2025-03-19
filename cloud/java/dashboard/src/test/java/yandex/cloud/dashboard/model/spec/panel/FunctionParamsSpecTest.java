package yandex.cloud.dashboard.model.spec.panel;

import org.junit.Assert;
import org.junit.Test;

import java.util.List;

/**
 * @author ssytnik
 */
public class FunctionParamsSpecTest {

    @Test
    public void parseParams() {
        Assert.assertEquals(List.of(), FunctionParamsSpec.parseParams(null));
        Assert.assertEquals(List.of(), FunctionParamsSpec.parseParams(""));
        Assert.assertEquals(List.of("a"), FunctionParamsSpec.parseParams("a"));
        Assert.assertEquals(List.of("a", "b"), FunctionParamsSpec.parseParams("a, b"));
        Assert.assertEquals(List.of("aa", "bb", "cc"), FunctionParamsSpec.parseParams("aa , bb , cc"));
        Assert.assertEquals(List.of("aa", "", "cc"), FunctionParamsSpec.parseParams("aa, , cc"));
    }

}