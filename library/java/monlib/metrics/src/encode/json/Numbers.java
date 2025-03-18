package ru.yandex.monlib.metrics.encode.json;

import java.nio.charset.StandardCharsets;


/**
 * @author Sergey Polovko
 */
@SuppressWarnings("Duplicates")
final class Numbers {
    private Numbers() {}

    private final static byte[] DigitTens = {
        '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
        '1', '1', '1', '1', '1', '1', '1', '1', '1', '1',
        '2', '2', '2', '2', '2', '2', '2', '2', '2', '2',
        '3', '3', '3', '3', '3', '3', '3', '3', '3', '3',
        '4', '4', '4', '4', '4', '4', '4', '4', '4', '4',
        '5', '5', '5', '5', '5', '5', '5', '5', '5', '5',
        '6', '6', '6', '6', '6', '6', '6', '6', '6', '6',
        '7', '7', '7', '7', '7', '7', '7', '7', '7', '7',
        '8', '8', '8', '8', '8', '8', '8', '8', '8', '8',
        '9', '9', '9', '9', '9', '9', '9', '9', '9', '9',
    };

    private final static byte[] DigitOnes = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    };

    private static final byte[] MinLongString = Long.toString(Long.MIN_VALUE)
        .getBytes(StandardCharsets.US_ASCII);

    // Requires positive x
    private static int longStringSize(long x) {
        long p = 10;
        for (int i = 1; i < 19; i++) {
            if (x < p) {
                return i;
            }
            p = 10 * p;
        }
        return 19;
    }

    public static int writeLong(long value, byte[] buf, int offset) {
        if (value == Long.MIN_VALUE) {
            System.arraycopy(MinLongString, 0, buf, offset, MinLongString.length);
            return offset + MinLongString.length;
        }

        if (value < 0) {
            buf[offset++] = '-';
            value = -value;
        }

        long q;
        int r;
        int charPos = (offset += longStringSize(value));

        // Get 2 digits/iteration using longs until quotient fits into an int
        while (value > Integer.MAX_VALUE) {
            q = value / 100;
            // really: r = value - (q * 100);
            r = (int)(value - ((q << 6) + (q << 5) + (q << 2)));
            value = q;
            buf[--charPos] = DigitOnes[r];
            buf[--charPos] = DigitTens[r];
        }

        // Get 2 digits/iteration using ints
        int q2;
        int i2 = (int)value;
        while (i2 >= 65536) {
            q2 = i2 / 100;
            // really: r = i2 - (q * 100);
            r = i2 - ((q2 << 6) + (q2 << 5) + (q2 << 2));
            i2 = q2;
            buf[--charPos] = DigitOnes[r];
            buf[--charPos] = DigitTens[r];
        }

        // Fall thru to fast mode for smaller numbers
        for (;;) {
            q2 = (i2 * 52429) >>> (16+3);
            r = i2 - ((q2 << 3) + (q2 << 1));  // r = i2-(q2*10) ...
            buf[--charPos] = DigitOnes[r];
            i2 = q2;
            if (i2 == 0) break;
        }
        return offset;
    }

    public static int writeDouble(double value, byte[] buf, int offset) {
        // TODO: optimize it
        String s = Double.toString(value);
        for (int i = 0; i < s.length(); i++) {
            buf[offset + i] = (byte) (s.charAt(i) & 0x7f);
        }
        return offset + s.length();
    }
}
