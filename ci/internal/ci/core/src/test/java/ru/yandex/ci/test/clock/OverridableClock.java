package ru.yandex.ci.test.clock;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.time.ZoneId;
import java.time.ZoneOffset;
import java.time.temporal.ChronoUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.UnaryOperator;

import javax.annotation.concurrent.ThreadSafe;

import com.google.common.base.Preconditions;

@ThreadSafe
public class OverridableClock extends Clock {

    private final Clock internalClock = Clock.tickMillis(ZoneOffset.UTC);
    private final AtomicReference<Instant> timeReference = new AtomicReference<>();

    public void setTime(Instant time) {
        this.timeReference.set(time);
    }

    public void setTimeSeconds(long seconds) {
        setTime(Instant.ofEpochSecond(seconds));
    }

    public void flush() {
        timeReference.set(null);
    }

    public void stop() {
        timeReference.set(internalClock.instant());
    }

    public void setEpoch() {
        setTime(Instant.EPOCH);
    }

    public Instant plusSeconds(int seconds) {
        return modifyTimeAndSpace(instant -> instant.plusSeconds(seconds));
    }

    public Instant plusHours(int hours) {
        return modifyTimeAndSpace(instant -> instant.plus(hours, ChronoUnit.HOURS));
    }

    public Instant plus(Duration duration) {
        return modifyTimeAndSpace(instant -> instant.plus(duration));
    }

    private Instant modifyTimeAndSpace(UnaryOperator<Instant> updateFunction) {
        return timeReference.updateAndGet(
                instant -> {
                    Preconditions.checkState(
                            instant != null,
                            "You should stop time first to control it. Use stop() or setTime() methods."
                    );
                    return updateFunction.apply(instant);
                }
        );
    }

    @Override
    public ZoneId getZone() {
        return internalClock.getZone();
    }

    @Override
    public Clock withZone(ZoneId zone) {
        throw new UnsupportedOperationException("You probably don't wont this for testing");
    }

    @Override
    public Instant instant() {
        Instant time = timeReference.get();
        return (time != null) ? time : internalClock.instant();
    }
}
