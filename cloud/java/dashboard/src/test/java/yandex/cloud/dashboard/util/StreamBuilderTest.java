package yandex.cloud.dashboard.util;

import org.junit.Assert;
import org.junit.Test;

import java.util.Arrays;

/**
 * @author ssytnik
 */
public class StreamBuilderTest {

    @Test
    public void overall() {
        Assert.assertEquals(
                Arrays.asList("a", "b", "c", "d", "e", "h"),
                StreamBuilder.create()
                        .add("a")
                        .add("b", "c")
                        .addIf(true, "d", "e")
                        .addIf(false, "f", "g")
                        .add("h")
                        .toList());
    }

}