package ru.yandex.monlib.metrics.series;

import java.util.Objects;

import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;

/**
 * @author Vladimir Gordiychuk
 */
abstract class HistogramTimeSeries extends TimeSeries {

    @Override
    public boolean isHistogram() {
        return true;
    }

    /**
     * ONE
     */
    static final class One extends HistogramTimeSeries {
        private final long tsMillis;
        private final HistogramSnapshot value;

        One(long tsMillis, HistogramSnapshot value) {
            this.tsMillis = tsMillis;
            this.value = value;
        }

        @Override
        public int size() {
            return 1;
        }

        @Override
        public long tsMillisAt(int index) {
            if (index != 0) {
                throw new IndexOutOfBoundsException("index != 0");
            }
            return tsMillis;
        }

        @Override
        public HistogramSnapshot histogramAt(int index) {
            if (index != 0) {
                throw new IndexOutOfBoundsException("index != 0");
            }
            return value;
        }

        @Override
        public TimeSeries addHistogram(long tsMillis, HistogramSnapshot value) {
            return new Many(5, this.tsMillis, this.value)
                    .addHistogram(tsMillis, value);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            One one = (One) o;
            return tsMillis == one.tsMillis
                    && value.boundsEquals(one.value)
                    && value.bucketsEquals(one.value);
        }

        @Override
        public int hashCode() {
            return Objects.hash(tsMillis, value);
        }

        @Override
        public String toString() {
            return "histograms[(" + tsMillis + ", " + value + ")]";
        }
    }

    /**
     * MANY
     */
    static final class Many extends HistogramTimeSeries {
        private long[] tsMillis;
        private HistogramSnapshot[] values;
        private int size;

        Many(int capacity) {
            tsMillis = new long[capacity];
            values = new HistogramSnapshot[capacity];
        }

        Many(int capacity, long firstTsMillis, HistogramSnapshot firstValue) {
            this(capacity);
            tsMillis[0] = firstTsMillis;
            values[0] = firstValue;
            size = 1;
        }

        @Override
        public int size() {
            return size;
        }

        @Override
        public long tsMillisAt(int index) {
            return tsMillis[index];
        }

        @Override
        public HistogramSnapshot histogramAt(int index) {
            return values[index];
        }

        @Override
        public TimeSeries addHistogram(long tsMillis, HistogramSnapshot value) {
            ensureCapacity();
            this.tsMillis[size] = tsMillis;
            this.values[size] = value;
            size++;
            return this;
        }

        private void ensureCapacity() {
            if (size >= this.tsMillis.length) {
                final int newSize = size + (size >> 1); // x1.5

                final long[] newTsMillis = new long[newSize];
                System.arraycopy(this.tsMillis, 0, newTsMillis, 0, size);
                this.tsMillis = newTsMillis;

                final HistogramSnapshot[] newValues = new HistogramSnapshot[newSize];
                System.arraycopy(this.values, 0, newValues, 0, size);
                this.values = newValues;
            }
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Many many = (Many) o;
            if (size != many.size) return false;
            for (int i = 0; i < size; i++) {
                if (tsMillis[i] != many.tsMillis[i]) return false;
                if (!values[i].boundsEquals(many.values[i])) return false;
                if (!values[i].bucketsEquals(many.values[i])) return false;
            }
            return true;
        }

        @Override
        public int hashCode() {
            int tsHash = 1, valueHash = 1;
            for (int i = 0; i < size; i++) {
                final long ts = tsMillis[i];
                tsHash = 31 * tsHash + (int) (ts ^ (ts >>> 32));

                final long value = values[i].hashCode();
                valueHash = 31 * valueHash + (int) (value ^ (value >>> 32));
            }
            return 31 * (31 * tsHash + valueHash) + size;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("histograms[");
            for (int i = 0; i < size; i++) {
                sb.append('(');
                sb.append(tsMillis[i]);
                sb.append(", ");
                sb.append(values[i]);
                sb.append("), ");
            }
            if (sb.length() > 2) {
                sb.setLength(sb.length() - 2);
            }
            sb.append(']');
            return sb.toString();
        }
    }
}
