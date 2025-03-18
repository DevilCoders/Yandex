package ru.yandex.monlib.metrics.labels.string;

import javax.annotation.ParametersAreNonnullByDefault;
import javax.annotation.concurrent.Immutable;

import ru.yandex.monlib.metrics.labels.Label;


/**
 * @author Sergey Polovko
 */
@Immutable
@ParametersAreNonnullByDefault
final class StringLabel implements Label {

    private final String key;
    private final String value;


    StringLabel(String key, String value) {
        this.key = key;
        this.value = value;
    }

    @Override
    public String getKey() {
        return key;
    }

    @Override
    public String getValue() {
        return value;
    }

    @Override
    public byte getKeyCharAt(int pos) {
        return (byte) (key.charAt(pos) & 0x7f);
    }

    @Override
    public byte getValueCharAt(int pos) {
        return (byte) (value.charAt(pos) & 0x7f);
    }

    @Override
    public int getKeyLength() {
        return key.length();
    }

    @Override
    public int getValueLength() {
        return value.length();
    }

    @Override
    public boolean hasKey(String key) {
        return this.key.equals(key);
    }

    @Override
    public boolean hasSameKey(Label label) {
        return this.key.equals(label.getKey());
    }

    @Override
    public int compareKey(String key) {
        return this.key.compareTo(key);
    }

    @Override
    public int compareKeys(Label label) {
        return this.key.compareTo(label.getKey());
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        StringLabel label = (StringLabel) o;
        return key.equals(label.key) && value.equals(label.value);
    }

    @Override
    public int hashCode() {
        int result = key.hashCode();
        result = 31 * result + value.hashCode();
        return result;
    }

    @Override
    public void toString(StringBuilder sb) {
        sb.ensureCapacity(key.length() + value.length() + 3);  // +3 for '=' and 2x'\''
        sb.append(key);
        sb.append('=');
        sb.append('\'');
        sb.append(value);
        sb.append('\'');
    }

    @Override
    public String toString() {
        final int additionalLen = 16; // len("StringLabel{" + '=' + 2x'\'' + '}')
        StringBuilder sb = new StringBuilder(additionalLen + key.length() + value.length());
        sb.append("StringLabel{");
        toString(sb);
        sb.append('}');
        return sb.toString();
    }
}
