package ru.yandex.ci.ydb.service;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;

import com.google.common.base.Stopwatch;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.db.CiMainDb;

import static org.assertj.core.api.Assertions.assertThat;

class CounterTableTest extends CommonYdbTestBase {

    private static final Logger log = LoggerFactory.getLogger(CounterTableTest.class);

    @Test
    void testSimple() {
        Assertions.assertTrue(db.currentOrTx(() -> db.counter().get("key1")).isEmpty());
        Assertions.assertEquals(1, db.currentOrTx(() -> db.counter().incrementAndGet("key1")));
        Assertions.assertEquals(2, db.currentOrTx(() -> db.counter().incrementAndGet("key1")));
        Assertions.assertEquals(3, db.currentOrTx(() -> db.counter().incrementAndGet("key1")));
        Assertions.assertEquals(3, db.currentOrTx(() -> db.counter().get("key1").orElseThrow()));
        Assertions.assertTrue(db.currentOrTx(() -> db.counter().get("key2")).isEmpty());
    }

    @Test
    void testParallel() throws Exception {
        String key = "p1";
        int updatesCount = 50;

        Stopwatch stopwatch = Stopwatch.createStarted();

        ExecutorService executorService = Executors.newFixedThreadPool(updatesCount / 10);

        List<Callable<Long>> callables = new ArrayList<>();

        for (int i = 0; i < updatesCount; i++) {
            callables.add(() -> db.currentOrTx(() -> db.counter().incrementAndGet(key)));
        }

        Set<Long> numbers = new HashSet<>();
        for (Future<Long> future : executorService.invokeAll(callables, 1, TimeUnit.MINUTES)) {
            numbers.add(future.get());
        }

        log.info(
                "{} parallel test executed in {} millis",
                updatesCount,
                stopwatch.stop().elapsed(TimeUnit.MILLISECONDS)
        );
        executorService.shutdown();

        Assertions.assertEquals(updatesCount, numbers.size());
        for (long i = 1; i <= updatesCount; i++) {
            Assertions.assertTrue(numbers.contains(i));
        }

        Assertions.assertEquals(updatesCount, db.currentOrTx(() -> db.counter().get(key)).orElseThrow());
    }

    @Test
    void testNamespace() {
        Assertions.assertTrue(db.currentOrTx(() -> db.counter().get("ns1", "key").isEmpty()));
        Assertions.assertTrue(db.currentOrTx(() -> db.counter().get("ns2", "key").isEmpty()));
        Assertions.assertEquals(1, db.currentOrTx(() -> db.counter().incrementAndGet("ns1", "key")));
        Assertions.assertEquals(2, db.currentOrTx(() -> db.counter().incrementAndGet("ns1", "key")));
        Assertions.assertEquals(1, db.currentOrTx(() -> db.counter().incrementAndGet("ns2", "key")));

        Assertions.assertEquals(2, db.currentOrTx(() -> db.counter().get("ns1", "key").orElseThrow()));
        Assertions.assertEquals(1, db.currentOrTx(() -> db.counter().get("ns2", "key").orElseThrow()));
        Assertions.assertTrue(db.currentOrTx(() -> db.counter().get("key").isEmpty()));
        Assertions.assertTrue(db.currentOrTx(() -> db.counter().get("ns3", "key").isEmpty()));
    }

    @Test
    void concurrent() throws InterruptedException {
        var counters = concurrent(() -> db.counter().incrementAndGet("concurrent-test"));

        assertThat(counters.getFinalValues()).containsExactlyInAnyOrder(1L, 2L);
        assertThat(counters.getAcquiredValues()).containsExactlyInAnyOrder(1L, 1L, 2L);
    }

    @Test
    void concurrentMinimal() throws InterruptedException {
        var counters = concurrent(
                () -> db.counter().incrementAndGetWithLowLimit("concurrent-minimal-test", "value", 42)
        );

        assertThat(counters.getFinalValues()).containsExactlyInAnyOrder(42L, 43L);
        assertThat(counters.getAcquiredValues()).containsExactlyInAnyOrder(42L, 42L, 43L);
    }

    @Test
    void dontBackward() {
        String ns = "concurrent-dont-backward";
        for (int i = 0; i < 3; i++) {
            db.currentOrTx(() -> db.counter().incrementAndGet(ns, "value"));
        }
        assertThat(db.currentOrTx(() -> db.counter().get(ns, "value")))
                .contains(3L);

        assertThat(db.currentOrTx(() -> db.counter().incrementAndGetWithLowLimit(ns, "value", 7)))
                .isEqualTo(7L);

        assertThat(db.currentOrTx(() -> db.counter().incrementAndGetWithLowLimit(ns, "value", 7)))
                .isEqualTo(8L);

        assertThat(db.currentOrTx(() -> db.counter().incrementAndGetWithLowLimit(ns, "value", 7)))
                .isEqualTo(9L);
    }

    @Test
    void concurrentForward() throws InterruptedException {
        String ns = "concurrent-forward";
        for (int i = 0; i < 13; i++) {
            db.currentOrTx(() -> db.counter().incrementAndGet(ns, "value"));
        }

        assertThat(db.currentOrTx(() -> db.counter().get(ns, "value")))
                .contains(13L);

        var counters = concurrent(() -> db.counter().incrementAndGetWithLowLimit(ns, "value", 98));

        assertThat(counters.getFinalValues()).containsExactlyInAnyOrder(98L, 99L);
        assertThat(counters.getAcquiredValues()).containsExactlyInAnyOrder(98L, 98L, 99L);

        assertThat(db.currentOrTx(() -> db.counter().get(ns, "value")))
                .contains(99L);
    }

    private ConcurrentResult concurrent(Supplier<Long> action) throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(2);

        var c1 = new Counter(db, latch, action);
        var c2 = new Counter(db, latch, action);

        c1.start();
        c2.start();

        c1.join();
        c2.join();

        return new ConcurrentResult(c1, c2);
    }

    @Value
    private static class ConcurrentResult {
        Counter c1;
        Counter c2;

        public List<Long> getFinalValues() {
            return List.of(c1.finalValue, c2.finalValue);
        }

        public List<Long> getAcquiredValues() {
            return ImmutableList.copyOf(Iterables.concat(c1.acquired, c2.acquired));
        }
    }

    @Slf4j
    private static class Counter extends Thread {
        private final CiMainDb db;
        private final CountDownLatch latch;
        private final Supplier<Long> action;
        private final List<Long> acquired = new ArrayList<>();
        private long finalValue;

        Counter(CiMainDb db, CountDownLatch latch, Supplier<Long> action) {
            this.db = db;
            this.latch = latch;
            this.action = action;
        }

        @Override
        public void run() {
            db.currentOrTx(() -> {
                try {
                    finalValue = action.get();
                    log.info("Got value {}", finalValue);
                    acquired.add(finalValue);
                    latch.countDown();
                    latch.await();

                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
            });

            log.info("Committed {}", finalValue);
        }
    }
}
