package ru.yandex.monlib.metrics.encode.spack.compression;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;


/**
 * @author Sergey Polovko
 */
public class DecodeStream {

    private final ByteBuffer in;


    DecodeStream(ByteBuffer in) {
        this.in = in.order(ByteOrder.LITTLE_ENDIAN);
    }

    public static DecodeStream create(CompressionAlg alg, ByteBuffer in) {
        switch (alg) {
            case NONE: return new DecodeStream(in);
            case ZLIB: return new ZlibDecodeStream(in);
            case ZSTD: return new ZstdDecodeStream(in);
            case LZ4: return new Lz4DecodeStream(in);
            default:
                throw new CompressionException("unsupported compression algorithm: " + alg);
        }
    }

    public byte readByte() {
        try {
            return in.get();
        } catch (IndexOutOfBoundsException e) {
            throw new CompressionException("stream unexpectedly ended");
        }
    }

    public int readIntLe() {
        try {
            return in.getInt();
        } catch (IndexOutOfBoundsException e) {
            throw new CompressionException("stream unexpectedly ended");
        }
    }

    public long readLongLe() {
        try {
            return in.getLong();
        } catch (IndexOutOfBoundsException e) {
            throw new CompressionException("stream unexpectedly ended");
        }
    }

    public double readDoubleLe() {
        return Double.longBitsToDouble(readLongLe());
    }

    public int readVarint32() {
        byte firstByte = readByte();
        if ((firstByte & 0x80) == 0) {
            return firstByte;
        }

        int result = firstByte & 0x7f;
        int offset = 7;
        for (; offset < 32; offset += 7) {
            final byte b = readByte();
            result |= (b & 0x7f) << offset;
            if ((b & 0x80) == 0) {
                return result;
            }
        }
        throw new CompressionException("too many bytes for varint32");
    }

    public int read(byte[] b, int off, int len) {
        if (!in.hasRemaining()) {
            return -1;
        }
        len = Math.min(in.remaining(), len);
        in.get(b, off, len);
        return len;
    }

    public void readFully(byte[] b, int len) {
        if (in.remaining() < len) {
            in.position(in.position() + in.remaining());
            throw new CompressionException("stream unexpectedly ended");
        }
        in.get(b, 0, len);
    }

    protected ByteBuffer getInputBuf() {
        return in;
    }
}
