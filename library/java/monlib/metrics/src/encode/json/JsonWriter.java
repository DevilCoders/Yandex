package ru.yandex.monlib.metrics.encode.json;

import java.io.IOException;
import java.io.OutputStream;

import ru.yandex.monlib.metrics.labels.Label;


/**
 * @author Sergey Polovko
 */
final class JsonWriter implements AutoCloseable {

    private final OutputStream out;
    private final byte[] buffer;
    private int position;

    private boolean commaExpected = false;


    public JsonWriter(OutputStream out, int bufferSize) {
        this.out = out;
        this.buffer = new byte[Math.max(bufferSize, 1024)];
    }

    public void objectBegin() {
        writeComma();
        commaExpected = false;
        write('{');
    }

    public void objectEnd() {
        write('}');
        commaExpected = true;
    }

    public void key(byte[] key) {
        writeComma();
        commaExpected = false;

        write('\"');
        writeString(key);
        write('\"');
        write(':');
    }

    public void keyValue(Label label) {
        writeComma();

        // (1) key
        write('\"');
        flushIfNeeded(label.getKeyLength());
        for (int i = 0; i < label.getKeyLength(); i++) {
            buffer[position++] = label.getKeyCharAt(i);
        }
        write('\"');
        write(':');

        // (2) value
        write('\"');
        flushIfNeeded(label.getValueLength());
        for (int i = 0; i < label.getValueLength(); i++) {
            buffer[position++] = label.getValueCharAt(i);
        }
        write('\"');
    }

    public void arrayBegin() {
        writeComma();
        commaExpected = false;
        write('[');
    }

    public void arrayEnd() {
        write(']');
        commaExpected = true;
    }

    public void stringValue(byte[] value) {
        writeComma();
        write('\"');
        writeString(value);
        write('\"');
    }

    public void numberValue(long value) {
        writeComma();
        flushIfNeeded(20);
        position = Numbers.writeLong(value, buffer, position);
    }

    public void numberValue(double value) {
        writeComma();
        flushIfNeeded(20);
        position = Numbers.writeDouble(value, buffer, position);
    }

    @Override
    public void close() {
        try {
            if (position != 0) {
                out.write(buffer, 0, position);
            }
            out.flush();
            out.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void writeComma() {
        if (commaExpected) {
            write(',');
        }
        commaExpected = true;
    }

    private void write(char ch) {
        flushIfNeeded(1);
        buffer[position++] = (byte) (ch & 0x7f);
    }

    private void writeString(byte[] value) {
        flushIfNeeded(value.length);
        System.arraycopy(value, 0, buffer, position, value.length);
        position += value.length;
    }

    private void flushIfNeeded(int requiredSize) {
        if ((position + requiredSize) >= buffer.length) {
            try {
                out.write(buffer, 0, position);
                position = 0;
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }
}
