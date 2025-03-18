package ru.yandex.monlib.metrics.encode.spack.compression;

import java.nio.ByteBuffer;

import com.github.luben.zstd.Zstd;


/**
 * @author Sergey Polovko
 */
class ZstdDecodeStream extends FrameDecodeStream {

    ZstdDecodeStream(ByteBuffer in) {
        super(in, new XxHash32Checksum());
    }

    @Override
    protected void decompress(byte[] compressed, int compressedSize, byte[] uncompressed, int uncompressedSize) {
        long size = Zstd.decompressByteArray(
            uncompressed, 0, uncompressedSize,
            compressed, 0, compressedSize);
        if (Zstd.isError(size)) {
            throw new CompressionException("zstd stream is corrupted: " + Zstd.getErrorName(size));
        } else if (size != uncompressedSize) {
            throw new CompressionException("zlib stream is corrupted: " + size + " != " + uncompressedSize);
        }
    }
}
