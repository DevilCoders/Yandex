package yandex.cloud.dashboard.model.spec.dashboard;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.dashboard.model.result.dashboard.Time;

import static org.junit.Assert.assertEquals;

/**
 * @author ssytnik
 */
public class TimeSpecTest {
    @Test
    public void overall() {
        valid(new Time("now-5m", "now"), new TimeSpec("5m"));
        valid(new Time("now-1h", "now"), new TimeSpec("1h"));
        valid(new Time("now-3h", "now"), new TimeSpec("3h"));
        valid(new Time("now/d", "now"), new TimeSpec("/d"));
        valid(new Time("now-2d/d", "now-2d/d"), new TimeSpec("2d/d", "2d/d"));
        valid(new Time("now/w", "now"), new TimeSpec("/w"));
        valid(new Time("now/w", "now/w"), new TimeSpec("/w", "/w"));
        valid(new Time("now-1M/M", "now-1M/M"), new TimeSpec("1M/M", "1M/M"));

        wrong(new TimeSpec(null));
        wrong(new TimeSpec("d"));
        wrong(new TimeSpec("/dd"));
        wrong(new TimeSpec("5x"));
    }

    private void valid(Time time, TimeSpec spec) {
        spec.validate(null);
        assertEquals(time, spec.asTime());
    }

    private void wrong(TimeSpec spec) {
        Assertions.assertThatExceptionOfType(IllegalArgumentException.class).isThrownBy(() -> spec.validate(null));
    }
}