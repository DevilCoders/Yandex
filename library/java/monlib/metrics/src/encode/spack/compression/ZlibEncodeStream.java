package ru.yandex.monlib.metrics.encode.spack.compression;

import java.io.OutputStream;
import java.util.zip.Deflater;


/**
 * @author Sergey Polovko
 */
public class ZlibEncodeStream extends FrameEncodeStream {

    private static final int LEVEL = 6;

    private final Deflater deflater;

    public ZlibEncodeStream(OutputStream out) {
        super(out, new Adler32Checksum());
        this.deflater = new Deflater(LEVEL, false);
    }

    @Override
    protected int maxCompressedSize(int uncompressedSize) {
        return compressBound(uncompressedSize);
    }

    @Override
    protected int compress(byte[] uncompressed, int uncompressedSize, byte[] compressed, int offset) {
        try {
            deflater.reset();
            deflater.setInput(uncompressed, 0, uncompressedSize);
            deflater.finish();
            return deflater.deflate(compressed, offset, compressed.length - offset);
        } catch (Exception e) {
            throw new CompressionException("cannot compress block with zlib", e);
        }
    }

    //
    // As always java(tm) sucks and there is no way to estimate how many bytes
    // deflater needs in destination buffer.
    //
    // Code copy-pasted from zlib sources:
    //   https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/zlib/compress.c?rev=3305705&blame=true#L75-80
    //
    private static int compressBound(int sourceLen) {
        return sourceLen + (sourceLen >>> 12) + (sourceLen >>> 14) + (sourceLen >>> 25) + 13;
    }
}
