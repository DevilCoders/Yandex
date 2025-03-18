package ru.yandex.monlib.metrics.encode.spack.compression;

import java.io.OutputStream;

import net.jpountz.lz4.LZ4Compressor;
import net.jpountz.lz4.LZ4Exception;
import net.jpountz.lz4.LZ4Factory;


/**
 * @author Sergey Polovko
 */
public class Lz4EncodeStream extends FrameEncodeStream {

    private final LZ4Compressor compressor;

    public Lz4EncodeStream(OutputStream out) {
        super(out, new XxHash32Checksum());
        this.compressor = LZ4Factory.fastestInstance().fastCompressor();
    }

    @Override
    protected int maxCompressedSize(int uncompressedSize) {
        return compressor.maxCompressedLength(uncompressedSize);
    }

    @Override
    protected int compress(byte[] uncompressed, int uncompressedSize, byte[] compressed, int offset) {
        try {
            return compressor.compress(
                uncompressed, 0, uncompressedSize,
                compressed, offset, compressed.length - offset);
        } catch (LZ4Exception e) {
            throw new CompressionException("cannot compress block with lz4", e);
        }
    }
}
