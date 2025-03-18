package ru.yandex.monlib.metrics.encode.spack;

import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.encode.MetricDecoder;
import ru.yandex.monlib.metrics.encode.spack.compression.DecodeStream;
import ru.yandex.monlib.metrics.encode.spack.format.MetricTypes;
import ru.yandex.monlib.metrics.encode.spack.format.MetricValuesType;
import ru.yandex.monlib.metrics.encode.spack.format.SpackHeader;
import ru.yandex.monlib.metrics.encode.spack.format.SpackVersion;
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision;
import ru.yandex.monlib.metrics.histogram.ExplicitHistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;


/**
 * @author Sergey Polovko
 */
public class MetricSpackDecoder implements MetricDecoder {

    private SpackHeader header;
    private DecodeStream in;
    private StringPool keysPool;
    private StringPool valuesPool;


    @Override
    public void decode(byte[] buffer, MetricConsumer consumer) {
        decode(ByteBuffer.wrap(buffer), consumer);
    }

    @Override
    public void decode(ByteBuffer buffer, MetricConsumer consumer) {
        // (1) header
        header = SpackHeader.readFrom(buffer);
        in = DecodeStream.create(header.getCompressionAlg(), buffer);
        consumer.onStreamBegin(header.getMetricCount());

        // (2) string pools
        readStringPools();

        // (3) common time
        consumer.onCommonTime(readTsMillis());

        // (4) common labels
        readLabels(consumer);

        // (5) metrics
        readMetrics(consumer);
        consumer.onStreamEnd();

        // (6) cleanup
        header = null;
        in = null;
        keysPool = valuesPool = null;
    }

    private void readMetrics(MetricConsumer consumer) {
        for (int i = 0; i < header.getMetricCount(); i++) {
            // (1) types byte
            final byte typesByte = in.readByte();
            final MetricType metricType = MetricTypes.metricType(typesByte);
            final MetricValuesType metricValuesType = MetricTypes.valuesType(typesByte);

            consumer.onMetricBegin(metricType);

            // (2) skip flags byte
            in.readByte();

            // (3) labels
            readLabels(consumer);

            // (4) points
            switch (metricValuesType) {
                case NONE:
                    break;

                case ONE_WITHOUT_TS: {
                    readValue(metricType, 0, consumer);
                    break;
                }

                case ONE_WITH_TS: {
                    final long tsMillis = readTsMillis();
                    readValue(metricType, tsMillis, consumer);
                    break;
                }

                case MANY_WITH_TS: {
                    final int count = in.readVarint32();
                    for (int j = 0; j < count; j++) {
                        final long tsMillis = readTsMillis();
                        readValue(metricType, tsMillis, consumer);
                    }
                    break;
                }

                default:
                    throw new SpackException("invalid metric values type: " + metricValuesType);
            }

            consumer.onMetricEnd();
        }
    }

    private void readStringPools() {
        final int maxPoolSize = Math.max(header.getLabelNamesSize(), header.getLabelValuesSize());
        final byte[] poolBuffer = new byte[maxPoolSize];

        in.readFully(poolBuffer, header.getLabelNamesSize());
        keysPool = new StringPool(poolBuffer, header.getLabelNamesSize());

        in.readFully(poolBuffer, header.getLabelValuesSize());
        valuesPool = new StringPool(poolBuffer, header.getLabelValuesSize());
    }

    private long readTsMillis() {
        return header.getTimePrecision() == TimePrecision.SECONDS
            ? TimeUnit.SECONDS.toMillis(in.readIntLe())
            : in.readLongLe();
    }

    private void readValue(MetricType metricType, long tsMillis, MetricConsumer consumer) {
        switch (metricType) {
            case DGAUGE:
                consumer.onDouble(tsMillis, in.readDoubleLe());
                break;
            case IGAUGE:
            case RATE:
            case COUNTER:
                consumer.onLong(tsMillis, in.readLongLe());
                break;
            case HIST:
            case HIST_RATE:
                consumer.onHistogram(tsMillis, readHistogram());
                break;
            default:
                throw new SpackException("Unsupported metric type: " + metricType);
        }
    }

    private HistogramSnapshot readHistogram() {
        int count = in.readVarint32();

        double[] bounds = new double[count];
        if (header.getVersion() == SpackVersion.v1_0) {
            // old format (bound as long)
            for (int index = 0; index < count; index++) {
                long bound = in.readLongLe();
                bounds[index] = (bound == Long.MAX_VALUE)
                    ? Histograms.INF_BOUND
                    : (double) bound;
            }
        } else {
            // new format (bound as double)
            for (int index = 0; index < count; index++) {
                bounds[index] = in.readDoubleLe();
            }
        }

        long[] buckets = new long[count];
        for (int index = 0; index < count; index++) {
            buckets[index] = in.readLongLe();
        }
        return new ExplicitHistogramSnapshot(bounds, buckets);
    }

    private void readLabels(MetricConsumer consumer) {
        final int count = in.readVarint32();
        if (count > 0) {
            consumer.onLabelsBegin(count);
            for (int i = 0; i < count; i++) {
                final int nameIdx = in.readVarint32();
                final int valueIdx = in.readVarint32();

                String key = keysPool.get(nameIdx);
                String value = valuesPool.get(valueIdx);
                consumer.onLabel(key, value);
            }
            consumer.onLabelsEnd();
        }
    }
}
