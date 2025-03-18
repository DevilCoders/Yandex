package ru.yandex.ci.common.temporal;

import java.util.Map;

import javax.annotation.Nullable;

import com.uber.m3.tally.Buckets;
import com.uber.m3.tally.Capabilities;
import com.uber.m3.tally.Counter;
import com.uber.m3.tally.Gauge;
import com.uber.m3.tally.Histogram;
import com.uber.m3.tally.Scope;
import com.uber.m3.tally.ScopeCloseException;
import com.uber.m3.tally.Stopwatch;
import com.uber.m3.tally.Timer;
import com.uber.m3.util.Duration;

/**
 * Temporal create timer instead of histogram. This created a lot of buckets (ex https://paste.yandex-team.ru/6803452)
 * TODO fix upstream CI-3241
 */
public class TemporalMetricScoreWrapper implements Scope {

    private final Scope wrapped;

    public TemporalMetricScoreWrapper(Scope wrapped) {
        this.wrapped = wrapped;
    }

    @Override
    public Timer timer(String name) {
        if (name.contains("_latency")) {
            return new HistogramTimer(histogram(name, null));
        }
        return wrapped.timer(name);
    }

    @Override
    public Scope tagged(Map<String, String> tags) {
        return new TemporalMetricScoreWrapper(wrapped.tagged(tags));
    }

    @Override
    public Scope subScope(String name) {
        return new TemporalMetricScoreWrapper(wrapped.subScope(name));
    }

    @Override
    public Counter counter(String name) {
        return wrapped.counter(name);
    }

    @Override
    public Gauge gauge(String name) {
        return wrapped.gauge(name);
    }

    @SuppressWarnings("deprecation")
    @Override
    public Histogram histogram(String name, @Nullable Buckets buckets) {
        return wrapped.histogram(name, buckets);
    }

    @Override
    public Capabilities capabilities() {
        return wrapped.capabilities();
    }

    @Override
    public void close() throws ScopeCloseException {
        wrapped.close();
    }

    private static class HistogramTimer implements Timer {
        private final Histogram histogram;

        private HistogramTimer(Histogram histogram) {
            this.histogram = histogram;
        }

        @Override
        public void record(Duration interval) {
            histogram.recordDuration(interval);
        }

        @Override
        public Stopwatch start() {
            return histogram.start();
        }
    }
}
