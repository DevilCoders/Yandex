package ru.yandex.ci.core.poller;

import java.util.Optional;
import java.util.concurrent.Callable;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.IntFunction;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

@ExtendWith(MockitoExtension.class)
class PollerTest {
    @Mock
    private ArcanumClientImpl arcanumClient;

    @Mock
    private Callable<Integer> action;

    @Mock
    private IntFunction<Long> interval;

    @BeforeEach
    void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    void testPollWithStatePredicate() throws Exception {
        ArcanumReviewDataDto.DiffSet diffSet1 = new ArcanumReviewDataDto.DiffSet(
                1, "gsid", "description", "status", "patchUrl", null, null
        );
        ArcanumReviewDataDto.DiffSet diffSet2 = new ArcanumReviewDataDto.DiffSet(
                2, "gsid", "description", "status", "patchUrl", null, null
        );
        Mockito.when(arcanumClient.getActiveDiffSet(Mockito.anyLong()))
                .thenReturn(Optional.empty(), Optional.of(diffSet1), Optional.of(diffSet2));

        PollerPredicate pollerPredicate = new PollerPredicate();

        ArcanumReviewDataDto.DiffSet polledDiffSet = Poller.poll(() -> arcanumClient.getActiveDiffSet(1))
                .canStopWhen(pollerPredicate::test)
                .allowIntervalLessThenOneSecond(true)
                .interval(100, TimeUnit.MILLISECONDS)
                .run()
                .orElseThrow();
        assertThat(polledDiffSet).isEqualTo(diffSet2);
    }

    @Test
    public void testRun() throws Throwable {
        Mockito.when(action.call()).thenReturn(1).thenReturn(2).thenReturn(3);

        Poller.poll(action)
                .canStopWhen(t -> t == 3)
                .interval(1, TimeUnit.MILLISECONDS)
                .allowIntervalLessThenOneSecond(true)
                .run();

        Mockito.verify(action, Mockito.times(3)).call();
    }

    @Test
    public void testIgnore() throws Throwable {
        Mockito.when(action.call())
                .thenReturn(1)
                .thenThrow(new IllegalArgumentException("testing ignore exception in builder"))
                .thenReturn(3);

        Poller.poll(action)
                .canStopWhen(t -> t == 3)
                .ignoring(IllegalArgumentException.class)
                .interval(1, TimeUnit.MILLISECONDS)
                .allowIntervalLessThenOneSecond(true)
                .run();

        Mockito.verify(action, Mockito.times(3)).call();
    }

    @Test
    public void testTimeout() throws Throwable {
        Mockito.when(action.call()).thenReturn(1).thenReturn(2).thenReturn(3);

        assertThatThrownBy(() ->
                Poller.poll(action)
                        .canStopWhen(t -> t == 3)
                        .timeout(100, TimeUnit.MILLISECONDS)
                        .interval(500, TimeUnit.MILLISECONDS)
                        .allowIntervalLessThenOneSecond(true)
                        .run()
        ).isInstanceOf(TimeoutException.class);
    }

    @Test
    public void testRetry() throws Throwable {
        Mockito.when(action.call())
                .thenThrow(new IllegalArgumentException("testing retry after exception in builder"))
                .thenReturn(2)
                .thenThrow(new IllegalArgumentException("testing retry after exception in builder"))
                .thenThrow(new IllegalArgumentException("testing retry after exception in builder"))
                .thenThrow(new IllegalArgumentException("testing retry after exception in builder"))
                .thenReturn(3);

        assertThatThrownBy(() ->
                Poller.poll(action)
                        .canStopWhen(t -> t == 3)
                        .retryOnExceptionCount(2)
                        .interval(1, TimeUnit.MILLISECONDS)
                        .allowIntervalLessThenOneSecond(true)
                        .run()
        ).isInstanceOf(RuntimeException.class);
    }

    @Test
    public void testRetryReset() throws Throwable {
        Mockito.when(action.call())
                .thenThrow(new IllegalArgumentException("testing retry reset after exception in builder"))
                .thenReturn(2)
                .thenThrow(new IllegalArgumentException("testing retry reset after exception in builder"))
                .thenReturn(3)
                .thenThrow(new IllegalArgumentException("testing retry reset after exception in builder"))
                .thenReturn(4)
                .thenThrow(new IllegalArgumentException("testing retry reset after exception in builder"))
                .thenReturn(5);

        Poller.poll(action)
                .canStopWhen(t -> t >= 5)
                .interval(1, TimeUnit.MILLISECONDS)
                .allowIntervalLessThenOneSecond(true)
                .retryOnExceptionCount(2)
                .run();
    }

    @Test
    public void testNotIgnoring() throws Throwable {
        Mockito.when(action.call())
                .thenThrow(new IllegalArgumentException("testing ignoring exception in builder"))
                .thenThrow(new IllegalStateException("testing not ignoring exception in builder"))
                .thenReturn(3);

        assertThatThrownBy(() ->
                Poller.poll(action)
                        .canStopWhen(t -> t > 2)
                        .retryOnExceptionCount(0)
                        .interval(1, TimeUnit.MILLISECONDS)
                        .allowIntervalLessThenOneSecond(true)
                        .ignoring(IllegalArgumentException.class)
                        .run()
        ).isInstanceOf(RuntimeException.class);
    }

    @Test
    public void testSleep() throws Throwable {
        Mockito.when(action.call()).thenReturn(1).thenReturn(2).thenReturn(3);
        Mockito.when(interval.apply(Mockito.anyInt())).thenReturn(1L);

        Poller.poll(action)
                .canStopWhen(t -> t.equals(3))
                .interval(interval, TimeUnit.MILLISECONDS)
                .allowIntervalLessThenOneSecond(true)
                .run();

        Mockito.verify(action, Mockito.times(3)).call();
        Mockito.verify(interval, Mockito.times(2)).apply(Mockito.anyInt());
    }

    @Test
    public void testOnThrow_SuccessfulRetry() throws Throwable {
        Mockito.when(action.call())
                .thenThrow(RuntimeException.class)
                .thenThrow(RuntimeException.class)
                .thenThrow(RuntimeException.class)
                .thenReturn(42);

        Integer result = Poller
                .poll(action)
                .onThrow(ex -> ex instanceof RuntimeException)
                .interval(retryCount -> 1L << retryCount, TimeUnit.NANOSECONDS)
                .allowIntervalLessThenOneSecond(true)
                .retryOnExceptionCount(5)
                .run();

        Assertions.assertThat(result).isEqualTo(Integer.valueOf(42));
        Mockito.verify(action, Mockito.times(4)).call();
    }

    @Test
    public void testOnThrow_UnsuccessfulRetry() throws Throwable {
        Mockito.when(action.call())
                .thenThrow(RuntimeException.class);

        assertThatThrownBy(() ->
                Poller.poll(action)
                        .onThrow(ex -> false)
                        .interval(retryCount -> 1L << retryCount, TimeUnit.NANOSECONDS)
                        .allowIntervalLessThenOneSecond(true)
                        .retryOnExceptionCount(5)
                        .run()
        ).isInstanceOf(RuntimeException.class);
    }

    @Test
    public void testOnThrow_UnretryableException() throws Throwable {
        Mockito.when(action.call())
                .thenThrow(IllegalArgumentException.class)
                .thenThrow(IllegalStateException.class);

        assertThatThrownBy(() ->
                Poller.poll(action)
                        .onThrow(ex -> ex instanceof IllegalArgumentException)
                        .interval(retryCount -> 1L << retryCount, TimeUnit.NANOSECONDS)
                        .allowIntervalLessThenOneSecond(true)
                        .retryOnExceptionCount(5)
                        .run()
        ).isInstanceOf(IllegalStateException.class);
    }

    @Test
    public void reduceSleepInterval_whenNextAttemptWillHappenAfterTimeout() throws Throwable {
        Mockito.when(action.call())
                .thenThrow(IllegalArgumentException.class);
        assertThatThrownBy(
                () -> Poller.poll(action)
                        .onThrow(ex -> ex instanceof IllegalArgumentException)
                        .timeout(1, TimeUnit.SECONDS)
                        .interval(1, TimeUnit.DAYS)
                        .retryOnExceptionCount(10)
                        .run()
        ).isInstanceOf(TimeoutException.class);

        Mockito.verify(action, Mockito.times(2)).call();
    }

    private static class PollerPredicate {
        private ArcanumReviewDataDto.DiffSet prevDiffSet = null;

        public boolean test(Optional<ArcanumReviewDataDto.DiffSet> currentDiffSet) {
            if (currentDiffSet.isPresent() && currentDiffSet.get().equals(prevDiffSet)) {
                return true;
            }

            prevDiffSet = currentDiffSet.orElse(null);

            return false;
        }
    }
}
