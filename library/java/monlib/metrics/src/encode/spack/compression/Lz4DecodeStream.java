package ru.yandex.monlib.metrics.encode.spack.compression;

import java.nio.ByteBuffer;

import net.jpountz.lz4.LZ4Exception;
import net.jpountz.lz4.LZ4Factory;
import net.jpountz.lz4.LZ4FastDecompressor;


/**
 * @author Sergey Polovko
 */
class Lz4DecodeStream extends FrameDecodeStream {

    private final LZ4FastDecompressor decompressor;


    Lz4DecodeStream(ByteBuffer in) {
        super(in, new XxHash32Checksum());
        this.decompressor = LZ4Factory.fastestInstance().fastDecompressor();
    }

    @Override
    protected void decompress(byte[] compressed, int compressedSize, byte[] uncompressed, int uncompressedSize) {
        try {
            int readBytes = decompressor.decompress(compressed, 0, uncompressed, 0, uncompressedSize);
            if (readBytes != compressedSize) {
                throw new CompressionException("lz4 stream is corrupted: " + readBytes + " != " + compressedSize);
            }
        } catch (LZ4Exception e) {
            throw new CompressionException("lz4 stream is corrupted", e);
        }
    }
}
