package ru.yandex.monlib.metrics.labels.validate;

import org.junit.Test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;


/**
 * @author Sergey Polovko
 */
public class LabelsValidatorTest {

    @Test
    public void isKeyValid() throws Exception {
        assertTrue(LabelsValidator.isKeyValid("a"));
        assertTrue(LabelsValidator.isKeyValid("a123"));
        assertTrue(LabelsValidator.isKeyValid("abcdefghijklmnopqrstuvwxyz123456"));
        assertTrue(LabelsValidator.isKeyValid("shard_id"));
        assertTrue(LabelsValidator.isKeyValid("a.b"));

        assertFalse(LabelsValidator.isKeyValid(""));
        assertFalse(LabelsValidator.isKeyValid("0a"));
        assertFalse(LabelsValidator.isKeyValid("a$"));
        assertFalse(LabelsValidator.isKeyValid("a#"));
        assertFalse(LabelsValidator.isKeyValid("a@"));
        assertFalse(LabelsValidator.isKeyValid("a-b"));
        assertFalse(LabelsValidator.isKeyValid("a/b"));
        assertFalse(LabelsValidator.isKeyValid("a*b"));
        assertFalse(LabelsValidator.isKeyValid("a+b"));
        assertFalse(LabelsValidator.isKeyValid("a=b"));
        assertFalse(LabelsValidator.isKeyValid("a!b"));
        assertFalse(LabelsValidator.isKeyValid("a(b)"));
        assertFalse(LabelsValidator.isKeyValid("a[b]"));
        assertFalse(LabelsValidator.isKeyValid("привет"));
        assertFalse(LabelsValidator.isKeyValid("abcdefghijklmnopqrstuvwxyz1234567890"));
    }

    @Test
    public void isValueValid() throws Exception {
        assertTrue(LabelsValidator.isValueValid("a"));
        assertTrue(LabelsValidator.isValueValid("a123"));
        assertTrue(LabelsValidator.isValueValid("0a"));
        assertTrue(LabelsValidator.isValueValid("a,b"));
        assertTrue(LabelsValidator.isValueValid("a:b"));
        assertTrue(LabelsValidator.isValueValid("a;b"));
        assertTrue(LabelsValidator.isValueValid("a-b"));
        assertTrue(LabelsValidator.isValueValid("a_b"));
        assertTrue(LabelsValidator.isValueValid("a.b"));
        assertTrue(LabelsValidator.isValueValid("a/b"));
        assertTrue(LabelsValidator.isValueValid("a(b)"));
        assertTrue(LabelsValidator.isValueValid("a[b]"));
        assertTrue(LabelsValidator.isValueValid("a<b>"));
        assertTrue(LabelsValidator.isValueValid("/a/b"));
        assertTrue(LabelsValidator.isValueValid("a@b"));

        assertFalse(LabelsValidator.isValueValid("some\u0001value"));
        assertFalse(LabelsValidator.isValueValid("привет"));
        assertFalse(LabelsValidator.isValueValid("-"));
        assertFalse(LabelsValidator.isValueValid("host-*"));
        assertFalse(LabelsValidator.isValueValid("???"));
        assertFalse(LabelsValidator.isValueValid("a|b"));
        assertFalse(LabelsValidator.isValueValid("\"value\""));
        assertFalse(LabelsValidator.isValueValid("'value'"));
        assertFalse(LabelsValidator.isValueValid("`value`"));
        assertFalse(LabelsValidator.isValueValid("\\w+"));
    }
}
