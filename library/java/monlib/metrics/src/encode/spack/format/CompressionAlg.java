package ru.yandex.monlib.metrics.encode.spack.format;

import javax.annotation.Nonnull;

import ru.yandex.monlib.metrics.encode.spack.SpackException;

/**
 * @author Sergey Polovko
 */
public enum CompressionAlg {
    NONE((byte) 0x00, ""),
    ZLIB((byte) 0x01, "gzip"),
    ZSTD((byte) 0x02, "zstd"),
    LZ4((byte) 0x03, "lz4"),
    ;

    private static final CompressionAlg[] VALUES = values();
    private static final String ZLIB_SYNONYM = "deflate";

    private final byte value;
    private final String encoding;

    CompressionAlg(byte value, String encoding) {
        this.value = value;
        this.encoding = encoding;
    }

    public byte value() {
        return value;
    }

    public static CompressionAlg valueOf(byte value) {
        for (CompressionAlg compressionAlg : VALUES) {
            if (compressionAlg.value == value) {
                return compressionAlg;
            }
        }
        throw new SpackException("unknown compression algorithm value: " + value);
    }

    public static CompressionAlg byEncoding(@Nonnull String encoding) {
        if (encoding.equals(ZLIB_SYNONYM)) {
            return ZLIB;
        }
        for (CompressionAlg compressionAlg : VALUES) {
            if (encoding.equals(compressionAlg.encoding)) {
                return compressionAlg;
            }
        }
        return NONE;
    }

    public String encoding() {
        return encoding;
    }
}
