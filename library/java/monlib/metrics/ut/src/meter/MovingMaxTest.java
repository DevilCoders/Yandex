package ru.yandex.monlib.metrics.meter;

import java.time.Duration;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.ParametersAreNonnullByDefault;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

/**
 * @author Ivan Tsybulin
 */
@ParametersAreNonnullByDefault
@Ignore
public class MovingMaxTest {
    private Duration window;
    private MaxMeter meter;

    @Before
    public void setUp() {
        window = Duration.ofSeconds(1);
        meter = new MaxMeter(0, window);
    }

    @Test
    public void threads() throws InterruptedException {
        Executor executor = Executors.newFixedThreadPool(4);
        int tasks = 10000;
        AtomicInteger doneTasks = new AtomicInteger(0);
        long startMillis = System.currentTimeMillis();
        long big = 10_000_000_000L;
        for (int i = 0; i < 10000; i++) {
            CompletableFuture.supplyAsync(() -> {
                long sleep = ThreadLocalRandom.current().nextLong(5, 15); // 10ms on avg
                long nowMillis = System.currentTimeMillis();
                double result = 0.0;
                while (System.currentTimeMillis() < nowMillis + sleep) {
                    result += Math.acos(result);
                }
                long value = ThreadLocalRandom.current().nextLong(0, big / (10 * window.toMillis() + nowMillis - startMillis));
                meter.tickIfNecessary();
                meter.record(value);
                return result;
            }, executor)
            .whenComplete((r, t) -> doneTasks.incrementAndGet());
        }
        Thread.sleep(2 * window.toMillis()); // warmup
        while (doneTasks.get() < tasks) {
            long max = meter.getMax();
            long denom = System.currentTimeMillis() - startMillis + 9 * window.toMillis();
            long value = max * denom;

            System.out.println(max + " " + value);
            Assert.assertEquals(value, big, 0.1 * big);

            Thread.sleep(1_000);
        }
    }
}
