package ru.yandex.ci.storage.core.utils;

import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;

import ru.yandex.ci.util.ExceptionUtils;

/**
 * Persistent hash code functions.
 */
public class CiHashCode {
    private CiHashCode() {

    }

    /**
     * Modified StringLatin1.hash()
     *
     * @param value value
     * @return hashcode
     */
    public static int hashCode(String value) {
        try {
            int h = 0;
            for (byte v : value.getBytes(StandardCharsets.UTF_8.name())) {
                h = 41 * h + (v & 0xff);
            }
            return h;
        } catch (UnsupportedEncodingException e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    public static int hashCode(Enum<?> value) {
        return hashCode(value.name());
    }

    public static int hashCode(Long value) {
        return (int) (value ^ (value >>> 32));
    }
}
