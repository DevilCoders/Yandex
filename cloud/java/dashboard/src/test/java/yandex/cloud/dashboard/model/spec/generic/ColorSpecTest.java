package yandex.cloud.dashboard.model.spec.generic;

import org.assertj.core.api.Assertions;
import org.junit.Assert;
import org.junit.Test;

/**
 * @author ssytnik
 */
public class ColorSpecTest {

    @Test
    public void parse() {
        Assert.assertEquals(new ColorSpec(0x33, 0x44, 0x55), ColorSpec.of("#345"));
        Assert.assertEquals(new ColorSpec(0xdd, 0xee, 0xff), ColorSpec.of("#def"));

        Assert.assertEquals(new ColorSpec(0x23, 0x45, 0x67), ColorSpec.of("#234567"));
        Assert.assertEquals(new ColorSpec(0xab, 0xcd, 0xef), ColorSpec.of("#abcdef"));

        Assertions.assertThatExceptionOfType(IllegalArgumentException.class).isThrownBy(() -> ColorSpec.of("qwe"));
        Assertions.assertThatExceptionOfType(IllegalArgumentException.class).isThrownBy(() -> ColorSpec.of("#aa"));
        Assertions.assertThatExceptionOfType(NumberFormatException.class).isThrownBy(() -> ColorSpec.of("#qwerty"));
    }

}