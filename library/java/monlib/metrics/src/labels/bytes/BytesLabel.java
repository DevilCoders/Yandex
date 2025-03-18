package ru.yandex.monlib.metrics.labels.bytes;

import java.util.Arrays;

import javax.annotation.ParametersAreNonnullByDefault;
import javax.annotation.concurrent.Immutable;

import ru.yandex.monlib.metrics.labels.Label;


/**
 * @author Sergey Polovko
 */
@Immutable
@ParametersAreNonnullByDefault
final class BytesLabel implements Label {

    private final byte[] buf;
    private final int valueOffset;

    public BytesLabel(String key, String value) {
        final int keyLen = key.length(); // key and value are always valid 7-bit ASCII strings
        final int valueLen = value.length();

        this.buf = new byte[keyLen + valueLen];
        int pos = 0;

        for (int i = 0; i < keyLen; i++) {
            buf[pos++] = (byte) (key.charAt(i) & 0x7f);
        }
        for (int i = 0; i < valueLen; i++) {
            buf[pos++] = (byte) (value.charAt(i) & 0x7f);
        }

        this.valueOffset = keyLen;
    }

    @Override
    public String getKey() {
        @SuppressWarnings("deprecation")
        final String str = new String(buf, 0, 0, getKeyLength());
        return str;
    }

    @Override
    public String getValue() {
        @SuppressWarnings("deprecation")
        final String str = new String(buf, 0, valueOffset, getValueLength());
        return str;
    }

    @Override
    public byte getKeyCharAt(int pos) {
        if (pos < 0 || pos >= getKeyLength()) {
            final String msg = String.format("index: %d, is out of [0, %d)", pos, getKeyLength());
            throw new IndexOutOfBoundsException(msg);
        }
        return buf[pos];
    }

    @Override
    public byte getValueCharAt(int pos) {
        if (pos < 0 || pos >= getValueLength()) {
            final String msg = String.format("index: %d, is out of [0, %d)", pos, getValueLength());
            throw new IndexOutOfBoundsException(msg);
        }
        return buf[pos + valueOffset];
    }

    @Override
    public int getKeyLength() {
        return valueOffset;
    }

    @Override
    public int getValueLength() {
        return buf.length - valueOffset;
    }

    @Override
    public boolean hasKey(String key) {
        if (key.length() != valueOffset) {
            return false;
        }
        for (int i = 0; i < key.length(); i++) {
            final byte ch = (byte) (key.charAt(i) & 0x7f);
            if (buf[i] != ch) {
                return false;
            }
        }
        return true;
    }

    @Override
    public boolean hasSameKey(Label label) {
        if (label instanceof BytesLabel) {
            final BytesLabel bytesLabel = (BytesLabel) label;
            if (valueOffset != bytesLabel.valueOffset) {
                return false;
            }
            for (int i = 0; i < valueOffset; i++) {
                if (buf[i] != bytesLabel.buf[i]) {
                    return false;
                }
            }
            return true;
        } else {
            return hasKey(label.getKey());
        }
    }

    @Override
    public int compareKey(String key) {
        final int len = Math.min(valueOffset, key.length()); // key is always valid 7-bit ASCII string
        for (int i = 0; i < len; i++) {
            final byte ch1 = buf[i];
            final byte ch2 = (byte) (key.charAt(i) & 0x7f);
            if (ch1 != ch2) {
                return ch1 - ch2;
            }
        }
        return valueOffset - key.length();
    }

    @Override
    public int compareKeys(Label label) {
        if (label instanceof BytesLabel) {
            final BytesLabel bytesLabel = (BytesLabel) label;
            final int len = Math.min(valueOffset, bytesLabel.valueOffset);
            for (int i = 0; i < len; i++) {
                final byte ch1 = buf[i];
                final byte ch2 = bytesLabel.buf[i];
                if (ch1 != ch2) {
                    return ch1 - ch2;
                }
            }
            return valueOffset - bytesLabel.valueOffset;
        } else {
            return compareKey(label.getKey());
        }
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        final BytesLabel that = (BytesLabel) o;
        return valueOffset == that.valueOffset && Arrays.equals(buf, that.buf);
    }

    @Override
    public int hashCode() {
        final int result = Arrays.hashCode(buf);
        return 31 * result + valueOffset;
    }

    @Override
    public void toString(StringBuilder sb) {
        sb.ensureCapacity(buf.length + 3); // +3 for '=' and 2x'\''
        for (int i = 0; i < valueOffset; i++) {
            sb.append((char) buf[i]);
        }
        sb.append('=');
        sb.append('\'');
        for (int i = valueOffset; i < buf.length; i++) {
            sb.append((char) buf[i]);
        }
        sb.append('\'');
    }

    @Override
    public String toString() {
        final int additionalLen = 15; // len("BytesLabel{" + '=' + 2x'\'' + '}')
        StringBuilder sb = new StringBuilder(additionalLen + buf.length);
        sb.append("BytesLabel{");
        toString(sb);
        sb.append('}');
        return sb.toString();
    }
}
