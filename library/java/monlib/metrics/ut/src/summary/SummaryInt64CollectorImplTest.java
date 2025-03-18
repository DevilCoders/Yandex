package ru.yandex.monlib.metrics.summary;

import java.util.LongSummaryStatistics;
import java.util.concurrent.ThreadLocalRandom;
import java.util.stream.LongStream;

import org.junit.Test;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertThat;

/**
 * @author Vladimir Gordiychuk
 */
public class SummaryInt64CollectorImplTest {

    @Test
    public void init() {
        SummaryInt64Collector collector = new SummaryInt64CollectorImpl();
        SummaryInt64Snapshot snapshot = collector.snapshot();
        assertEquals(0, snapshot.getCount());
        assertEquals(0, snapshot.getSum());
        assertEquals(Long.MIN_VALUE, snapshot.getMax());
        assertEquals(Long.MAX_VALUE, snapshot.getMin());
        assertEquals(0, snapshot.getLast());
    }

    @Test
    public void collect() {
        SummaryInt64Collector collector = new SummaryInt64CollectorImpl();
        collector.collect(42L);
        SummaryInt64Snapshot s1 = collector.snapshot();

        assertThat(s1.getCount(), equalTo(1L));
        assertThat(s1.getSum(), equalTo(42L));
        assertThat(s1.getMin(), equalTo(42L));
        assertThat(s1.getMax(), equalTo(42L));
        assertEquals(42, s1.getLast());

        collector.collect(5L);
        collector.collect(7L);
        collector.collect(1L);

        SummaryInt64Snapshot s2 = collector.snapshot();
        assertThat(s2.getCount(), equalTo(4L));
        assertThat(s2.getSum(), equalTo(42L + 5L + 7L + 1L));
        assertThat(s2.getMax(), equalTo(42L));
        assertThat(s2.getMin(), equalTo(1L));
        assertEquals(1, s2.getLast());
    }

    @Test
    public void parallelCollect() {
        SummaryInt64Collector collector = new SummaryInt64CollectorImpl();
        LongSummaryStatistics streamStat = LongStream.generate(() -> ThreadLocalRandom.current().nextLong(1, 1000))
                .parallel()
                .limit(1000)
                .peek(collector::collect)
                .summaryStatistics();

        SummaryInt64Snapshot snapshot = collector.snapshot();
        assertThat(snapshot.getCount(), equalTo(streamStat.getCount()));
        assertThat(snapshot.getMin(), equalTo(streamStat.getMin()));
        assertThat(snapshot.getMax(), equalTo(streamStat.getMax()));
        assertNotEquals(0, snapshot.getLast());
    }
}
