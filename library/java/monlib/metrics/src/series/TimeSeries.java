package ru.yandex.monlib.metrics.series;

import javax.annotation.CheckReturnValue;

import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;

/**
 * @author Sergey Polovko
 */
public abstract class TimeSeries {

    public boolean isDouble() {
        return false;
    }

    public boolean isLong() {
        return false;
    }

    public boolean isHistogram() {
        return false;
    }

    public boolean isEmpty() {
        return size() == 0;
    }

    public abstract int size();

    public abstract long tsMillisAt(int index);

    public double doubleAt(int index) {
        throw new IllegalStateException("it is not a double timeseries");
    }

    public long longAt(int index) {
        throw new IllegalStateException("it is not a long timeseries");
    }

    public HistogramSnapshot histogramAt(int index) {
        throw new IllegalStateException("it is not a histogram timeseries");
    }

    @CheckReturnValue
    public TimeSeries addDouble(long tsMillis, double value) {
        throw new IllegalStateException("it is not a double timeseries");
    }

    @CheckReturnValue
    public TimeSeries addLong(long tsMillis, long value) {
        throw new IllegalStateException("it is not a long timeseries");
    }

    @CheckReturnValue
    public TimeSeries addHistogram(long tsMillis, HistogramSnapshot value) {
        throw new IllegalStateException("it is not a histogram timeseries");
    }

    /**
     * empty
     */
    public static TimeSeries empty() {
        return EmptyTimeSeries.SELF;
    }

    /**
     * double
     */
    public static TimeSeries newDouble(long tsMillis, double value) {
        return new DoubleTimeSeries.One(tsMillis, value);
    }

    public static TimeSeries newDouble(int capacity) {
        return new DoubleTimeSeries.Many(capacity);
    }

    public static TimeSeries newDouble(int capacity, long tsMillis, double value) {
        return new DoubleTimeSeries.Many(capacity, tsMillis, value);
    }

    /**
     * long
     */
    public static TimeSeries newLong(long tsMillis, long value) {
        return new LongTimeSeries.One(tsMillis, value);
    }

    public static TimeSeries newLong(int capacity) {
        return new LongTimeSeries.Many(capacity);
    }

    public static TimeSeries newLong(int capacity, long tsMillis, long value) {
        return new LongTimeSeries.Many(capacity, tsMillis, value);
    }

    /**
     * histogram
     */
    public static TimeSeries newHistogram(long tsMillis, HistogramSnapshot value) {
        return new HistogramTimeSeries.One(tsMillis, value);
    }

    public static TimeSeries newHistogram(int capacity) {
        return new HistogramTimeSeries.Many(capacity);
    }
}
