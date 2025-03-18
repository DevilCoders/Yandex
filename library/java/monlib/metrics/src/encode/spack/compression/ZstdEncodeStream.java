package ru.yandex.monlib.metrics.encode.spack.compression;

import java.io.OutputStream;

import com.github.luben.zstd.Zstd;


/**
 * @author Sergey Polovko
 */
public class ZstdEncodeStream extends FrameEncodeStream {

    private static final int LEVEL = 11;


    public ZstdEncodeStream(OutputStream out) {
        super(out, new XxHash32Checksum());
    }

    @Override
    protected int maxCompressedSize(int uncompressedSize) {
        return Math.toIntExact(Zstd.compressBound(uncompressedSize));
    }

    @Override
    protected int compress(byte[] uncompressed, int uncompressedSize, byte[] compressed, int offset) {
        long compressedSize = Zstd.compressByteArray(
            compressed, offset, compressed.length - offset,
            uncompressed, 0, uncompressedSize,
            LEVEL);
        if (Zstd.isError(compressedSize)) {
            throw new CompressionException("cannot compress block with zstd: " + Zstd.getErrorName(compressedSize));
        }
        return Math.toIntExact(compressedSize);
    }
}
