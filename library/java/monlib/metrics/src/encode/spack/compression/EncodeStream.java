package ru.yandex.monlib.metrics.encode.spack.compression;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import ru.yandex.monlib.metrics.encode.spack.SpackException;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;
import ru.yandex.monlib.metrics.encode.spack.format.SpackHeader;


/**
 * @author Sergey Polovko
 */
public class EncodeStream implements AutoCloseable {

    private final OutputStream out;
    private final ByteBuffer buffer;

    EncodeStream(OutputStream out, int bufferSize) {
        this.out = out;
        this.buffer = ByteBuffer.allocate(bufferSize)
            .order(ByteOrder.LITTLE_ENDIAN);
    }

    public static EncodeStream create(CompressionAlg alg, OutputStream in) {
        switch (alg) {
            case NONE: return new EncodeStream(in, 8 << 10); // 8 KiB
            case ZLIB: return new ZlibEncodeStream(in);
            case ZSTD: return new ZstdEncodeStream(in);
            case LZ4: return new Lz4EncodeStream(in);
            default:
                throw new CompressionException("unsupported compression algorithm: " + alg);
        }
    }

    public void writeHeader(SpackHeader header) {
        if (buffer.position() != 0) {
            throw new SpackException("you are trying to write header in the middle of the stream");
        }
        header.writeTo(buffer);

        // skip compression for header
        buffer.flip();
        writeImpl(buffer);
        buffer.clear();
    }

    public void writeByte(byte value) {
        flushIfNeeded(1);
        buffer.put(value);
    }

    public void writeShortLe(short value) {
        flushIfNeeded(2);
        buffer.putShort(value);
    }

    public void writeIntLe(int value) {
        flushIfNeeded(4);
        buffer.putInt(value);
    }

    public void writeLongLe(long value) {
        flushIfNeeded(8);
        buffer.putLong(value);
    }

    public void writeDouble(double value) {
        writeLongLe(Double.doubleToLongBits(value));
    }

    public void writeString(String value) {
        flushIfNeeded(value.length() + 1);
        // TODO: check that string is a 7-bit ASCII string
        for (int i = 0; i < value.length(); i++) {
            buffer.put((byte) (value.charAt(i) & 0x7f));
        }
        buffer.put((byte) 0);
    }

    public void writeVarint32(int value) {
        flushIfNeeded(5);
        while (true) {
            if ((value & ~0x7f) == 0) {
                buffer.put((byte) value);
                return;
            } else {
                buffer.put((byte) ((value & 0x7f) | 0x80));
                value >>>= 7;
            }
        }
    }

    public void write(byte[] buffer, int offset, int length) {
        while (length != 0) {
            final int remaining = this.buffer.remaining();
            if (length < remaining) {
                this.buffer.put(buffer, offset, length);
                return;
            }

            this.buffer.put(buffer, offset, remaining);
            offset += remaining;
            length -= remaining;

            doFlush(this.buffer);
        }
    }

    protected final void writeImpl(ByteBuffer buffer) {
        try {
            out.write(buffer.array(), buffer.arrayOffset(), buffer.remaining());
        } catch (IOException e) {
            throw new RuntimeException("cannot write data into underlying stream", e);
        }
    }

    private void flushIfNeeded(int i) {
        if (buffer.remaining() < i) {
            doFlush(buffer);
        }
    }

    protected void doFlush(ByteBuffer buffer) {
        buffer.flip();
        writeImpl(buffer);
        buffer.clear();
    }

    protected void doFinish(ByteBuffer buffer) {
        if (buffer.position() > 0) {
            doFlush(buffer);
        }
    }

    @Override
    public void close() {
        try {
            doFinish(buffer);
            out.flush();
            out.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
