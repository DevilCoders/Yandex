package ru.yandex.monlib.metrics.histogram;

import org.junit.Assert;
import org.junit.Test;

/**
 * @author Sergey Polovko
 */
public class DoublesTest {

    @Test
    public void doubleToString() {
        Assert.assertEquals("30", Doubles.toString(30.0));

        Assert.assertEquals("-100000000", Doubles.toString(-100000000.0));
        Assert.assertEquals("-10000000", Doubles.toString(-10000000.0));
        Assert.assertEquals("-1000000", Doubles.toString(-1000000.0));
        Assert.assertEquals("-100000", Doubles.toString(-100000.0));
        Assert.assertEquals("-10000", Doubles.toString(-10000.0));
        Assert.assertEquals("-1000", Doubles.toString(-1000.0));
        Assert.assertEquals("-100", Doubles.toString(-100.0));
        Assert.assertEquals("-10", Doubles.toString(-10.0));
        Assert.assertEquals("-1", Doubles.toString(-1.0));
        Assert.assertEquals("0", Doubles.toString(0.0));
        Assert.assertEquals("1", Doubles.toString(1.0));
        Assert.assertEquals("10", Doubles.toString(10.0));
        Assert.assertEquals("100", Doubles.toString(100.0));
        Assert.assertEquals("1000", Doubles.toString(1000.0));
        Assert.assertEquals("10000", Doubles.toString(10000.0));
        Assert.assertEquals("100000", Doubles.toString(100000.0));
        Assert.assertEquals("1000000", Doubles.toString(1000000.0));
        Assert.assertEquals("10000000", Doubles.toString(10000000.0));
        Assert.assertEquals("100000000", Doubles.toString(100000000.0));

        Assert.assertEquals("-0.123456789", Doubles.toString(-0.123456789));
        Assert.assertEquals("-0.12345678", Doubles.toString(-0.12345678));
        Assert.assertEquals("-0.1234567", Doubles.toString(-0.1234567));
        Assert.assertEquals("-0.123456", Doubles.toString(-0.123456));
        Assert.assertEquals("-0.12345", Doubles.toString(-0.12345));
        Assert.assertEquals("-0.1234", Doubles.toString(-0.1234));
        Assert.assertEquals("-0.123", Doubles.toString(-0.123));
        Assert.assertEquals("-0.12", Doubles.toString(-0.12));
        Assert.assertEquals("-0.1", Doubles.toString(-0.1));
        Assert.assertEquals("0.1", Doubles.toString(0.1));
        Assert.assertEquals("0.12", Doubles.toString(0.12));
        Assert.assertEquals("0.123", Doubles.toString(0.123));
        Assert.assertEquals("0.1234", Doubles.toString(0.1234));
        Assert.assertEquals("0.12345", Doubles.toString(0.12345));
        Assert.assertEquals("0.123456", Doubles.toString(0.123456));
        Assert.assertEquals("0.1234567", Doubles.toString(0.1234567));
        Assert.assertEquals("0.12345678", Doubles.toString(0.12345678));
        Assert.assertEquals("0.123456789", Doubles.toString(0.123456789));

        Assert.assertEquals("9007199254740990", Doubles.toString((1L << 53) - 2));
        Assert.assertEquals("9007199254740991", Doubles.toString((1L << 53) - 1));
        Assert.assertEquals("9007199254740992", Doubles.toString((1L << 53)));

        // expected precision lost
        Assert.assertEquals("9007199254740992", Doubles.toString((1L << 53) + 1));
        Assert.assertEquals("9007199254740994", Doubles.toString((1L << 53) + 2));
        Assert.assertEquals("9007199254740996", Doubles.toString((1L << 53) + 3));

        // special values
        Assert.assertEquals("Infinity", Doubles.toString(Double.POSITIVE_INFINITY));
        Assert.assertEquals("-Infinity", Doubles.toString(Double.NEGATIVE_INFINITY));
        Assert.assertEquals("NaN", Doubles.toString(Double.NaN));
    }
}
