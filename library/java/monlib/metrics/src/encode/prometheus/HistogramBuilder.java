package ru.yandex.monlib.metrics.encode.prometheus;

import java.util.Arrays;
import java.util.Collections;
import java.util.Map;

import javax.annotation.Nullable;

import ru.yandex.monlib.metrics.histogram.ExplicitHistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;


/**
 * @author Sergey Polovko
 */
final class HistogramBuilder {

    private static final int INITIAL_SIZE = 10;

    private String name = "";
    @Nullable
    private Map<String, String> labels = null;
    private long tsMillis = 0;
    private double[] bounds = new double[INITIAL_SIZE];
    private long[] buckets = new long[INITIAL_SIZE];
    private int count = 0;
    private double prevBound = -Double.MAX_VALUE;
    private long prevBucket = 0;

    void clear() {
        name = "";
        labels = null;
        tsMillis = 0;
        count = 0;
        prevBound = -Double.MAX_VALUE;
        prevBucket = 0;
    }

    String getName() {
        return name;
    }

    void setName(String name) {
        this.name = name;
    }

    Map<String, String> getLabels() {
        return labels == null ? Collections.emptyMap() : labels;
    }

    void setLabels(Map<String, String> labels) {
        if (this.labels != null) {
            if (!this.labels.equals(labels)) {
                throw new IllegalStateException("mixed labels in one histogram, prev: " + this.labels + ", current: " + labels);
            }
        } else {
            this.labels = labels;
        }
    }

    long getTsMillis() {
        return tsMillis;
    }

    void setTsMillis(long tsMillis) {
        this.tsMillis = tsMillis;
    }

    boolean isEmpty() {
        return count == 0;
    }

    boolean isSame(String name, Map<String, String> labels) {
        return name.equals(this.name) && labels.equals(this.labels);
    }

    void addBucket(double bound, long bucket) {
        if (prevBound >= bound) {
            throw new IllegalStateException("invalid order of histogram bounds " + prevBound + " >= " + bound);
        }

        if (prevBucket > bucket) {
            throw new IllegalStateException("invalid order of histogram bucket values " + prevBucket + " > " + bucket);
        }

        // ensure arrays capacity
        final int i = count++;
        if (i == bounds.length) {
            final int newSize = bounds.length + (bounds.length >>> 1); // x1.5
            bounds = Arrays.copyOf(bounds, newSize);
            buckets = Arrays.copyOf(buckets, newSize);
        }

        // convert infinite bound value
        if (bound == Double.POSITIVE_INFINITY) {
            bound = Histograms.INF_BOUND;
        }

        bounds[i] = bound;
        buckets[i] = bucket - prevBucket; // keep only delta between buckets

        prevBound = bound;
        prevBucket = bucket;
    }

    HistogramSnapshot toSnapshot() {
        if (isEmpty()) {
            throw new IllegalStateException("histogram cannot be empty");
        }
        return new ExplicitHistogramSnapshot(Arrays.copyOf(bounds, count), Arrays.copyOf(buckets, count));
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(256);
        sb.append("HistogramBuilder{");
        sb.append("name=").append(name);
        sb.append(", labels=").append(labels);
        sb.append(", tsMillis=").append(tsMillis);
        sb.append(", bounds=[");
        for (int i = 0; i < count; i++) {
            if (i > 0) {
                sb.append(", ");
            }
            sb.append(bounds[i]);
        }
        sb.append("], buckets=[");
        for (int i = 0; i < count; i++) {
            if (i > 0) {
                sb.append(", ");
            }
            sb.append(buckets[i]);
        }
        sb.append("], count=");
        sb.append(count);
        sb.append("}");
        return sb.toString();
    }
}
