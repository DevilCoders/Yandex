package ru.yandex.monlib.metrics.series;

/**
 * @author Sergey Polovko
 */
abstract class LongTimeSeries extends TimeSeries {

    @Override
    public boolean isLong() {
        return true;
    }

    /**
     * ONE
     */
    static final class One extends LongTimeSeries {
        private final long tsMillis;
        private final long value;

        One(long tsMillis, long value) {
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
        public long longAt(int index) {
            if (index != 0) {
                throw new IndexOutOfBoundsException("index != 0");
            }
            return value;
        }

        @Override
        public TimeSeries addLong(long tsMillis, long value) {
            return new Many(5, this.tsMillis, this.value)
                .addLong(tsMillis, value);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            One one = (One) o;
            return tsMillis == one.tsMillis && value == one.value;
        }

        @Override
        public int hashCode() {
            final int result = (int) (tsMillis ^ (tsMillis >>> 32));
            return 31 * result + (int) (value ^ (value >>> 32));
        }

        @Override
        public String toString() {
            return "longs[(" + tsMillis + ", " + value + ")]";
        }
    }

    /**
     * MANY
     */
    static final class Many extends LongTimeSeries {
        private long[] tsMillis;
        private long[] values;
        private int size;

        Many(int capacity) {
            tsMillis = new long[capacity];
            values = new long[capacity];
        }

        Many(int capacity, long firstTsMillis, long firstValue) {
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
        public long longAt(int index) {
            return values[index];
        }

        @Override
        public TimeSeries addLong(long tsMillis, long value) {
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

                final long[] newValues = new long[newSize];
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
                if (values[i] != many.values[i]) return false;
            }
            return true;
        }

        @Override
        public int hashCode() {
            int tsHash = 1, valueHash = 1;
            for (int i = 0; i < size; i++) {
                final long ts = tsMillis[i];
                tsHash = 31 * tsHash + (int)(ts ^ (ts >>> 32));

                final long value = values[i];
                valueHash = 31 * valueHash + (int)(value ^ (value >>> 32));
            }
            return 31 * (31 * tsHash + valueHash) + size;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("longs[");
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
