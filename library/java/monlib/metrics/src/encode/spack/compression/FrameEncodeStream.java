package ru.yandex.monlib.metrics.encode.spack.compression;

import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
 * Look at {@link FrameDecodeStream} javadoc for a frame structure.
 *
 * @author Sergey Polovko
 */
public abstract class FrameEncodeStream extends EncodeStream {

    private static final int MAX_FRAME_SIZE = 2 << 20; // 2 MiB
    private static final int DEFAULT_FRAME_SIZE = 64 << 10; // 64 KiB
    private static final int FRAME_HEADER_SIZE = 4 + 4;
    private static final int FRAME_FOOTER_SIZE = 4;

    private final Checksum checksumAlg;

    private ByteBuffer frame = ByteBuffer.allocate(0);


    FrameEncodeStream(OutputStream out, Checksum checksumAlg) {
        super(out, DEFAULT_FRAME_SIZE);
        this.checksumAlg = checksumAlg;
    }

    @Override
    protected void doFlush(ByteBuffer uncompressed) {
        uncompressed.flip();
        writeCompressedFrame(uncompressed);
        uncompressed.clear();
    }

    @Override
    protected void doFinish(ByteBuffer uncompressed) {
        if (uncompressed.position() > 0) {
            uncompressed.flip();
            writeCompressedFrame(uncompressed);
            uncompressed.clear();
        }
        writeEmptyFrame();
    }

    private void writeCompressedFrame(ByteBuffer uncompressed) {
        final int uncompressedSize = uncompressed.remaining();
        final int maxFrameSize = FRAME_HEADER_SIZE + FRAME_FOOTER_SIZE + maxCompressedSize(uncompressedSize);

        // sanity check
        if (maxFrameSize > MAX_FRAME_SIZE) {
            throw new CompressionException("frame is too huge: " + maxFrameSize + " bytes, max is " + MAX_FRAME_SIZE);
        }
        ensureFrameCapacity(maxFrameSize);

        // (1) compress
        final int compressedSize = compress(uncompressed.array(), uncompressedSize, frame.array(), FRAME_HEADER_SIZE);

        // (2) add header
        frame.putInt(compressedSize);
        frame.putInt(uncompressedSize);

        // (3) add footer
        final int checkSum = checksumAlg.calc(
            frame.array(),
            frame.arrayOffset() + FRAME_HEADER_SIZE,
            compressedSize);
        frame.position(FRAME_HEADER_SIZE + compressedSize);
        frame.putInt(checkSum);

        // (4) write
        frame.flip();
        writeImpl(frame);
    }

    private void writeEmptyFrame() {
        ensureFrameCapacity(FRAME_HEADER_SIZE + FRAME_FOOTER_SIZE);
        frame.putInt(0);
        frame.putInt(0);
        frame.putInt(0);
        frame.flip();
        writeImpl(frame);
    }

    private void ensureFrameCapacity(int frameSize) {
        if (frame.capacity() < frameSize) {
            frame = ByteBuffer.allocate(frameSize)
                .order(ByteOrder.LITTLE_ENDIAN);
        } else {
            frame.clear();
            frame.limit(frameSize);
        }
    }

    protected abstract int maxCompressedSize(int uncompressedSize);
    protected abstract int compress(byte[] uncompressed, int uncompressedSize, byte[] compressed, int offset);
}
