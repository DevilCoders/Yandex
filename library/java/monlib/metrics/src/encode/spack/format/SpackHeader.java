package ru.yandex.monlib.metrics.encode.spack.format;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import ru.yandex.monlib.metrics.encode.spack.SpackException;


/**
 * @author Sergey Polovko
 */
public class SpackHeader {
    /**
     * defines how many bytes used for header by current implementation
     */
    public static final short HEADER_SIZE = 24;

    /**
     * expected format magic number
     */
    private static final short VALID_MAGIC = 0x5053; // "SP" in LE-order

    private final SpackVersion version;
    private final short headerSize;
    private final TimePrecision timePrecision;
    private final CompressionAlg compressionAlg;
    private final int labelNamesSize;
    private final int labelValuesSize;
    private final int metricCount;
    private final int pointCount;


    private SpackHeader(ByteBuffer in) {
        if (in.remaining() < HEADER_SIZE) {
            throw new SpackException(
                "not enough bytes in buffer to read spack header, " +
                "need at least: " + HEADER_SIZE + ", but got: " + in.remaining());
        }

        in = in.order(ByteOrder.LITTLE_ENDIAN);

        final int magic = in.getShort();
        if (magic != VALID_MAGIC) {
            throw new SpackException("invalid magic, expected " +
                Integer.toString(VALID_MAGIC, 16) + ", got " + Integer.toString(magic, 16));
        }

        short versionNum = in.getShort();
        SpackVersion version = SpackVersion.byValue(versionNum);
        if (version == null) {
            throw new SpackException("invalid version number: " + versionNum);
        }
        this.version = version;

        this.headerSize = in.getShort();
        if (this.headerSize < HEADER_SIZE) {
            throw new SpackException("wrong header size field value, expected >= " +
                HEADER_SIZE + ", got " + this.headerSize);
        }

        this.timePrecision = TimePrecision.valueOf(in.get());
        this.compressionAlg = CompressionAlg.valueOf(in.get());
        this.labelNamesSize = in.getInt();
        this.labelValuesSize = in.getInt();
        this.metricCount = in.getInt();
        this.pointCount = in.getInt();

        final int toSkip = this.headerSize - HEADER_SIZE;
        if (toSkip > 0) {
            in.position(in.position() + toSkip);
        }
    }

    public SpackHeader(
        TimePrecision timePrecision,
        CompressionAlg compressionAlg,
        int labelNamesSize, int labelValuesSize,
        int metricCount, int pointCount)
    {
        this.version = SpackVersion.CURRENT;
        this.headerSize = HEADER_SIZE;
        this.timePrecision = timePrecision;
        this.compressionAlg = compressionAlg;
        this.labelNamesSize = labelNamesSize;
        this.labelValuesSize = labelValuesSize;
        this.metricCount = metricCount;
        this.pointCount = pointCount;
    }

    public static SpackHeader readFrom(ByteBuffer in) {
        return new SpackHeader(in);
    }

    public void writeTo(ByteBuffer out) {
        out.order(ByteOrder.LITTLE_ENDIAN);
        out.putShort(VALID_MAGIC);
        out.putShort(SpackVersion.CURRENT.getValue());
        out.putShort(headerSize);
        out.put(timePrecision.value());
        out.put(compressionAlg.value());
        out.putInt(labelNamesSize);
        out.putInt(labelValuesSize);
        out.putInt(metricCount);
        out.putInt(pointCount);
    }

    public SpackVersion getVersion() {
        return version;
    }

    public short getHeaderSize() {
        return headerSize;
    }

    public TimePrecision getTimePrecision() {
        return timePrecision;
    }

    public CompressionAlg getCompressionAlg() {
        return compressionAlg;
    }

    public int getLabelNamesSize() {
        return labelNamesSize;
    }

    public int getLabelValuesSize() {
        return labelValuesSize;
    }

    public int getMetricCount() {
        return metricCount;
    }

    public int getPointCount() {
        return pointCount;
    }
}
