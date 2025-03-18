package ru.yandex.monlib.metrics.encode.spack.compression;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
 * Frame structure:
 *
 * +-----------------+-------------------+============+---------------+
 * | compressed size | uncompressed size |    data    |   check sum   |
 * +-----------------+-------------------+============+---------------+
 *    4 bytes           4 bytes             var len       4 bytes
 *
 * @author Sergey Polovko
 */
abstract class FrameDecodeStream extends DecodeStream {

    private static final int MAX_UNCOMPRESSED_SIZE = 512 << 10; // 512 KiB
    private static final int MAX_COMPRESSED_SIZE = MAX_UNCOMPRESSED_SIZE;

    private final Checksum checksumAlg;

    /**
     * used as storage for compressed data of the last read block
     */
    private byte[] compressedBlock = new byte[1024];

    /**
     * used as storage for decompressed data of the last read block
     */
    private ByteBuffer uncompressedBlock = ByteBuffer.allocate(0);

    /**
     * used when reading primitive types located on the border of neighbouring blocks, thus must have
     * enough capacity to read them
     */
    private final byte[] smallBuf = new byte[8];


    FrameDecodeStream(ByteBuffer in, Checksum checksumAlg) {
        super(in);
        this.checksumAlg = checksumAlg;
        uncompressedBlock.order(ByteOrder.LITTLE_ENDIAN);
    }

    @Override
    public byte readByte() {
        if (uncompressedBlock.hasRemaining()) {
            // fast pass
            return uncompressedBlock.get();
        }
        if (!decodeNextFrame()) {
            return -1;
        }
        return uncompressedBlock.get();
    }

    @Override
    public int readIntLe() {
        if (uncompressedBlock.remaining() >= 4) {
            // fast pass
            return uncompressedBlock.getInt();
        }
        readFully(smallBuf, 4);
        return
            (smallBuf[0] & 0xff)       |
            (smallBuf[1] & 0xff) << 8  |
            (smallBuf[2] & 0xff) << 16 |
            (smallBuf[3] & 0xff) << 24;
    }

    @Override
    public long readLongLe() {
        if (uncompressedBlock.remaining() >= 8) {
            // fast pass
            return uncompressedBlock.getLong();
        }
        readFully(smallBuf, 8);
        return
            ((long) smallBuf[0] & 0xff)       |
            ((long) smallBuf[1] & 0xff) <<  8 |
            ((long) smallBuf[2] & 0xff) << 16 |
            ((long) smallBuf[3] & 0xff) << 24 |
            ((long) smallBuf[4] & 0xff) << 32 |
            ((long) smallBuf[5] & 0xff) << 40 |
            ((long) smallBuf[6] & 0xff) << 48 |
            ((long) smallBuf[7] & 0xff) << 56;
    }

    @Override
    public int read(byte[] b, int off, int len) {
        if (!uncompressedBlock.hasRemaining()) {
            if (!decodeNextFrame()) {
                return -1;
            }
        }
        len = Math.min(uncompressedBlock.remaining(), len);
        uncompressedBlock.get(b, off, len);
        return len;
    }

    @Override
    public void readFully(byte[] b, int len) {
        int read = 0;
        while (read < len) {
            final int r = read(b, read, len - read);
            if (r < 0) {
                throw new CompressionException("stream unexpectedly ended");
            }
            read += r;
        }
    }

    /**
     * @return true  - if input buffer is ended
     *         false - otherwise
     */
    private boolean decodeNextFrame() {
        final ByteBuffer in = getInputBuf();
        if (!in.hasRemaining()) {
            return false;
        }

        int compressedSize = in.getInt();
        if (compressedSize > MAX_COMPRESSED_SIZE) {
            throw new CompressionException(
                "too huge compressed block " + compressedSize +
                    " bytes, max size " + MAX_COMPRESSED_SIZE + " bytes");
        }

        int uncompressedSize = in.getInt();
        if (uncompressedSize > MAX_UNCOMPRESSED_SIZE) {
            throw new CompressionException(
                "too huge uncompressed block " + uncompressedSize +
                " bytes, max size " + MAX_UNCOMPRESSED_SIZE + " bytes");
        }

        if (compressedBlock.length < compressedSize) {
            compressedBlock = new byte[compressedSize];
        }

        uncompressedBlock.clear();
        if (uncompressedBlock.capacity() < uncompressedSize) {
            uncompressedBlock = ByteBuffer.allocate(uncompressedSize)
                .order(ByteOrder.LITTLE_ENDIAN);
        }

        in.get(compressedBlock, 0, compressedSize);

        int checksum = in.getInt();
        checksumAlg.check(compressedBlock, 0, compressedSize, checksum);

        decompress(compressedBlock, compressedSize, uncompressedBlock.array(), uncompressedSize);
        uncompressedBlock.limit(uncompressedSize);
        return true;
    }

    protected abstract void decompress(byte[] compressed, int compressedSize, byte[] uncompressed, int uncompressedSize);
}
