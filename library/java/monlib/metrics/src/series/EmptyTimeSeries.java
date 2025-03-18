package ru.yandex.monlib.metrics.series;

import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;

/**
 * @author Sergey Polovko
 */
final class EmptyTimeSeries extends TimeSeries {

    public static final TimeSeries SELF = new EmptyTimeSeries();

    private EmptyTimeSeries() {
    }

    @Override
    public boolean isDouble() {
        return true;
    }

    @Override
    public boolean isLong() {
        return true;
    }

    @Override
    public boolean isEmpty() {
        return true;
    }

    @Override
    public boolean isHistogram() {
        return true;
    }

    @Override
    public int size() {
        return 0;
    }

    @Override
    public long tsMillisAt(int index) {
        throw new IndexOutOfBoundsException("it is empty timeseries");
    }

    @Override
    public double doubleAt(int index) {
        throw new IndexOutOfBoundsException("it is empty timeseries");
    }

    @Override
    public long longAt(int index) {
        throw new IndexOutOfBoundsException("it is empty timeseries");
    }

    @Override
    public HistogramSnapshot histogramAt(int index) {
        throw new IndexOutOfBoundsException("it is empty timeseries");
    }

    @Override
    public TimeSeries addDouble(long tsMillis, double value) {
        return new DoubleTimeSeries.One(tsMillis, value);
    }

    @Override
    public TimeSeries addLong(long tsMillis, long value) {
        return new LongTimeSeries.One(tsMillis, value);
    }

    @Override
    public TimeSeries addHistogram(long tsMillis, HistogramSnapshot value) {
        return new HistogramTimeSeries.One(tsMillis, value);
    }

    @Override
    public String toString() {
        return "[]";
    }
}
