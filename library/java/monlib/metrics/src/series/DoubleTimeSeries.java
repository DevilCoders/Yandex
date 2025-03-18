package ru.yandex.monlib.metrics.series;

/**
 * @author Sergey Polovko
 */
abstract class DoubleTimeSeries extends TimeSeries {

    @Override
    public boolean isDouble() {
        return true;
    }

    /**
     * ONE
     */
    static final class One extends DoubleTimeSeries {
        private final long tsMillis;
        private final double value;

        One(long tsMillis, double value) {
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
        public double doubleAt(int index) {
            if (index != 0) {
                throw new IndexOutOfBoundsException("index != 0");
            }
            return value;
        }

        @Override
        public TimeSeries addDouble(long tsMillis, double value) {
            return new Many(5, this.tsMillis, this.value)
                .addDouble(tsMillis, value);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            One one = (One) o;
            return tsMillis == one.tsMillis && Double.compare(one.value, value) == 0;
        }

        @Override
        public int hashCode() {
            final int result = (int) (tsMillis ^ (tsMillis >>> 32));
            final long temp = Double.doubleToLongBits(value);
            return 31 * result + (int) (temp ^ (temp >>> 32));
        }

        @Override
        public String toString() {
            return "doubles[(" + tsMillis + ", " + value + ")]";
        }
    }

    /**
     * MANY
     */
    static final class Many extends DoubleTimeSeries {
        private long[] tsMillis;
        private double[] values;
        private int size;

        Many(int capacity) {
            tsMillis = new long[capacity];
            values = new double[capacity];
        }

        Many(int capacity, long firstTsMillis, double firstValue) {
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
        public double doubleAt(int index) {
            return values[index];
        }

        @Override
        public TimeSeries addDouble(long tsMillis, double value) {
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

                final double[] newValues = new double[newSize];
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
                if (Double.compare(values[i], many.values[i]) != 0) return false;
            }
            return true;
        }

        @Override
        public int hashCode() {
            int tsHash = 1, valueHash = 1;
            for (int i = 0; i < size; i++) {
                final long ts = tsMillis[i];
                tsHash = 31 * tsHash + (int)(ts ^ (ts >>> 32));

                final long value = Double.doubleToLongBits(values[i]);
                valueHash = 31 * valueHash + (int)(value ^ (value >>> 32));
            }
            return 31 * (31 * tsHash + valueHash) + size;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("doubles[");
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
