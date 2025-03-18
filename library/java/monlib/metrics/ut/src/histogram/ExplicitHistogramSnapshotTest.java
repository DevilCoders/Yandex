package ru.yandex.monlib.metrics.histogram;

import org.junit.Test;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.junit.Assert.assertThat;

/**
 * @author Vladimir Gordiychuk
 */
public class ExplicitHistogramSnapshotTest {

    @Test
    public void toStringEmpty() {
        assertThat(ExplicitHistogramSnapshot.EMPTY.toString(), equalTo("{}"));
    }

    @Test
    public void toStringSingle() {
        HistogramSnapshot snapshot = new ExplicitHistogramSnapshot(new double[]{100}, new long[]{42});
        assertThat(snapshot.toString(), equalTo("{100: 42}"));
    }

    @Test
    public void toString3Item() {
        HistogramSnapshot snapshot = new ExplicitHistogramSnapshot(new double[]{10.0, 15.0, 30.0}, new long[]{5, 7, 9});
        assertThat(snapshot.toString(), equalTo("{10: 5, 15: 7, 30: 9}"));
    }

    @Test
    public void toStringWithInf() {
        HistogramSnapshot snapshot = new ExplicitHistogramSnapshot(
            new double[]{5, 10, 15, 30, Histograms.INF_BOUND},
            new long[]{1, 2, 3, 4, 5});
        assertThat(snapshot.toString(), equalTo("{5: 1, 10: 2, 15: 3, 30: 4, inf: 5}"));
    }
}
