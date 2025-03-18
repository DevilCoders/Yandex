package ru.yandex.monlib.metrics.labels.validate;

import java.util.Objects;


/**
 * @author Sergey Polovko
 */
final class CharsPattern {

    private  static final int MAX_ASCII_CHARS = 0x80;

    private final int maxLen;
    private final boolean[] firstChars;
    private final boolean[] restChars;

    private CharsPattern(int maxLen, boolean[] firstChars, boolean[] restChars) {
        if (maxLen <= 0) {
            throw new IllegalArgumentException("maxLen(" + maxLen + ") is invalid");
        }
        this.maxLen = maxLen;
        this.firstChars = Objects.requireNonNull(firstChars, "firstChars");
        this.restChars = Objects.requireNonNull(restChars, "restChars");
    }

    public boolean isValidLen(int len) {
        return len > 0 && len <= maxLen;
    }

    public int getMaxLen() {
        return maxLen;
    }

    public boolean isValidFirst(char c) {
        return isAscii(c) && firstChars[c];
    }

    public boolean isValidRest(char c) {
        return isAscii(c) && restChars[c];
    }

    /**
     * Checks for a character value that fits in the 7-bit ASCII set.
     */
    static boolean isAscii(char c) {
        return (c & 0xff80) == 0;
    }

    static final class Builder {
        private final boolean[] firstChars = new boolean[MAX_ASCII_CHARS];
        private final boolean[] restChars = new boolean[MAX_ASCII_CHARS];
        private int maxLen = -1;

        Builder maxLen(int maxLen) {
            this.maxLen = maxLen;
            return this;
        }

        Builder addFirst(char from, char to) {
            for (char c = from; c <= to; c++) {
                firstChars[c] = true;
            }
            return this;
        }

        Builder addRest(char from, char to) {
            for (char c = from; c <= to; c++) {
                restChars[c] = true;
            }
            return this;
        }

        Builder add(char from, char to) {
            for (char c = from; c <= to; c++) {
                firstChars[c] = restChars[c] = true;
            }
            return this;
        }

        Builder addFirst(String chars) {
            for (char c : chars.toCharArray()) {
                firstChars[c] = true;
            }
            return this;
        }

        Builder addRest(String chars) {
            for (char c : chars.toCharArray()) {
                restChars[c] = true;
            }
            return this;
        }

        Builder add(String chars) {
            for (char c : chars.toCharArray()) {
                firstChars[c] = restChars[c] = true;
            }
            return this;
        }

        Builder ignore(String chars) {
            for (char c : chars.toCharArray()) {
                firstChars[c] = restChars[c] = false;
            }
            return this;
        }

        CharsPattern build() {
            return new CharsPattern(maxLen, firstChars, restChars);
        }
    }
}
