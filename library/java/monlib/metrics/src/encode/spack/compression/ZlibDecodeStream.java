package ru.yandex.monlib.metrics.encode.spack.compression;

import java.nio.ByteBuffer;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;


/**
 * @author Sergey Polovko
 */
class ZlibDecodeStream extends FrameDecodeStream {

    private final Inflater inflater;


    ZlibDecodeStream(ByteBuffer in) {
        super(in, new Adler32Checksum());
        this.inflater = new Inflater(false);
    }

    @Override
    protected void decompress(byte[] compressed, int compressedSize, byte[] uncompressed, int uncompressedSize) {
        try {
            inflater.reset();
            inflater.setInput(compressed, 0, compressedSize);
            int uncompressedBytes = inflater.inflate(uncompressed);
            if (uncompressedBytes != uncompressedSize) {
                throw new CompressionException("zlib stream is corrupted: " + uncompressedBytes + " != " + uncompressedSize);
            }
            if (!inflater.finished()) {
                throw new CompressionException("zlib stream is corrupted: not properly finished");
            }
        } catch (DataFormatException e) {
            throw new CompressionException("zlib stream is corrupted", e);
        }
    }
}
