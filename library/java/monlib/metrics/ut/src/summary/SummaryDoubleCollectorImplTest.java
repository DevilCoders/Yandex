package ru.yandex.monlib.metrics.summary;

import java.util.DoubleSummaryStatistics;
import java.util.concurrent.ThreadLocalRandom;
import java.util.stream.DoubleStream;

import org.junit.Test;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertThat;

/**
 * @author Vladimir Gordiychuk
 */
public class SummaryDoubleCollectorImplTest {

    @Test
    public void init() {
        SummaryDoubleCollector collector = new SummaryDoubleCollectorImpl();
        SummaryDoubleSnapshot snapshot = collector.snapshot();
        assertThat(snapshot.getCount(), equalTo(0L));
        assertThat(snapshot.getSum(), equalTo(0d));
        assertThat(snapshot.getMax(), equalTo(Double.NEGATIVE_INFINITY));
        assertThat(snapshot.getMin(), equalTo(Double.POSITIVE_INFINITY));
        assertEquals(0, snapshot.getLast(), 0);
    }

    @Test
    public void collect() {
        SummaryDoubleCollector collector = new SummaryDoubleCollectorImpl();
        collector.collect(42d);
        SummaryDoubleSnapshot s1 = collector.snapshot();

        assertThat(s1.getCount(), equalTo(1L));
        assertThat(s1.getSum(), equalTo(42d));
        assertThat(s1.getMin(), equalTo(42d));
        assertThat(s1.getMax(), equalTo(42d));
        assertEquals(42.0, s1.getLast(), 0);

        collector.collect(5d);
        collector.collect(7.5d);
        collector.collect(1d);

        SummaryDoubleSnapshot s2 = collector.snapshot();
        assertThat(s2.getCount(), equalTo(4L));
        assertThat(s2.getSum(), equalTo(42d + 5d + 7.5d + 1d));
        assertThat(s2.getMax(), equalTo(42d));
        assertThat(s2.getMin(), equalTo(1d));
        assertEquals(1.0, s2.getLast(), 0);
    }

    @Test
    public void parallelCollect() {
        SummaryDoubleCollector collector = new SummaryDoubleCollectorImpl();
        DoubleSummaryStatistics streamStat = DoubleStream.generate(() -> (double) ThreadLocalRandom.current().nextLong(1, 1000))
                .parallel()
                .limit(1000)
                .peek(collector::collect)
                .summaryStatistics();

        SummaryDoubleSnapshot snapshot = collector.snapshot();
        assertThat(snapshot.getCount(), equalTo(streamStat.getCount()));
        assertThat(snapshot.getMin(), equalTo(streamStat.getMin()));
        assertThat(snapshot.getMax(), equalTo(streamStat.getMax()));
        assertNotEquals(0, snapshot.getLast(), 0);
    }
}
