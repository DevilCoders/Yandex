package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.time.Duration;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.base.Stopwatch;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.locks.InterProcessLock;
import org.apache.curator.framework.recipes.locks.InterProcessMutex;

import yandex.cloud.repository.db.Tx;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.util.ExceptionUtils;
import ru.yandex.lang.NonNullApi;

/**
 * Класс был создан для борьбы с большим количеством OptimisticLockException, которые возникают, когда много кубиков
 * в одном не релизном флоу пытаются проапдейтить одну строку во FlowLaunch.
 * <p>
 * Обычный flow и action'ы независимы, один запущеннный флоу никогда не попытается захватить лок другого флоу.
 * Релизные флоу таким свойством не обладают.
 */
@Slf4j
@NonNullApi
public class FlowLaunchMutexManager {

    @Nonnull
    private final CuratorFramework curator;
    @Nonnull
    private final LoadingCache<FlowLaunchId, Mutex> flowLaunchIdMutex;
    @Nonnull
    private final Duration acquireMutexTimeout;

    public FlowLaunchMutexManager(
            @Nonnull CuratorFramework curator,
            int mutexMapInitCapacity,
            int concurrencyLevel,
            @Nonnull Duration acquireMutexTimeout,
            Duration oldMutexesCleanupTimeout
    ) {
        this.curator = curator;
        this.flowLaunchIdMutex = CacheBuilder.newBuilder()
                .initialCapacity(mutexMapInitCapacity)
                .concurrencyLevel(concurrencyLevel)
                .expireAfterAccess(oldMutexesCleanupTimeout)
                .build(CacheLoader.from(this::createMutex));
        this.acquireMutexTimeout = acquireMutexTimeout;
    }

    public void acquireAndRun(LaunchId launchId, Runnable runnable) {
        acquireAndRun(launchId, () -> {
            runnable.run();
            return null;
        });
    }

    public <T> T acquireAndRun(LaunchId launchId, Supplier<T> supplier) {
         /*  Для релизов лок по CiProcessId не подходит, потому что
                мы не хотим блокировать соседние запуски одного релизного процесса.

            Для релизов лок по flowLaunchId не подходит, потому что
                - Релиз R1 при освобождении стадии может в этой же транзакции захватить освобождённую
                    стадию для следующего релиза R2. При этом будет попытка захвата лока для R2 внутри транзакции,
                    что плохо. Наш код в этом случае выкинет исключение.
                - Релиз R2 может попытаться захватить лок для релиза R1 в случае вытеснения релизов.
                    То есть мы даже не можем гарантировать порядок захвата локов. А это потенциальный deadlock.

            Для релизов подойдёт лок по flowLaunchId при условии, что текущий тред не захватил ещё ни одного лока.
            Так мы избежим deadlock'ов и покроем кейс, когда кубики одного релиза падают с OptimisticLockException.
        */
        if (ReleaseThreadLocalLock.isAcquiredByCurrentThread()) {
            // Если лок уже захвачен, то не смотрим на zk-лок во избежание deadlock'ов
            return supplier.get();
        }

        var processId = launchId.getProcessId();
        log.info("Trying to acquire action: {}", processId);
        Mutex mutex;
        try {
            mutex = flowLaunchIdMutex.get(FlowLaunchId.of(launchId));
        } catch (ExecutionException e) {
            var unwrapped = ExceptionUtils.unwrap(e);
            log.error("Failed to get mutex for {}", FlowLaunchId.of(launchId), unwrapped);
            throw unwrapped;
        }

        try (var releaseLock = ReleaseThreadLocalLock.acquire(processId)) {
            return acquireAndRun(mutex, supplier);
        }
    }

    private <T> T acquireAndRun(Mutex mutex, Supplier<T> supplier) {
        try (var acquiredMutex = acquireOrThrow(mutex)) {
            return supplier.get();
        }
    }

    private Mutex acquireOrThrow(Mutex mutex) {
        if (!mutex.localLock.isHeldByCurrentThread()) {
            /* Проверяем, только если лок захватывается первый раз. Не проверяем в случае повторного захвата
               (который возможен, так как это ReentrantLock). */
            Preconditions.checkState(!Tx.Current.exists(),
                    "Mutex should be acquired outside of transaction at first time!");
        }

        mutex.acquireOrThrow();
        return mutex;
    }

    private Mutex createMutex(FlowLaunchId flowLaunchId) {
        return createMutex(zkPath(flowLaunchId));
    }

    private Mutex createMutex(String zkPath) {
        log.info("Creating mutex for {}", zkPath);
        return createMutex(zkPath, new ReentrantLock(), createZkLock(zkPath));
    }

    @VisibleForTesting
    protected Mutex createMutex(String zkPath, ReentrantLock localLock, InterProcessLock zkLock) {
        return new Mutex(zkPath, localLock, zkLock);
    }

    protected InterProcessLock createZkLock(String zkPath) {
        return new InterProcessMutex(curator, zkPath);
    }

    private static String zkPath(FlowLaunchId flowLaunchId) {
        return "/flow_launch_mutex/flow_launch_id/" + flowLaunchId.asString();
    }

    /**
     * Порядок захвата и освобождения локов:
     * - захватываем сначала локальный, затем zk
     * - освобождаем сначала zk, затем локальный
     * Такой порядок даёт возможность другому серверу захватить zk-лок и начать выполнять работу
     */
    @RequiredArgsConstructor
    public class Mutex implements AutoCloseable {

        @Nonnull
        private final String zkPath;
        @Nonnull
        private final ReentrantLock localLock;
        @Nonnull
        private final InterProcessLock zkLock;

        public void acquireOrThrow() {
            runWithTryCatch(this::doAcquireOrThrow);
        }

        private Void doAcquireOrThrow() throws Exception {
            boolean acquiredZkLock = false;

            var stopWatch = Stopwatch.createStarted();
            if (!acquireLocalLock()) {
                throw new RuntimeException("Cannot acquire local lock within %d msec: %s".formatted(
                        acquireMutexTimeout.toMillis(), zkPath
                ));
            }

            try {
                var elapsedMillis = stopAndGetElapsedMillis(stopWatch);
                log.info("Local lock acquired within {} msec: flowLaunchId {}", elapsedMillis, zkPath);

                var timeoutMillis = reduceTimeout(elapsedMillis);

                stopWatch.reset().start();
                if (!acquireZkLock(timeoutMillis)) {
                    throw new RuntimeException("Cannot acquire zk lock within %d msec: %s".formatted(
                            timeoutMillis, zkPath
                    ));
                }

                acquiredZkLock = true;

                log.info("Zk lock acquired within {} msec: {}", stopAndGetElapsedMillis(stopWatch), zkPath);
            } finally {
                if (!acquiredZkLock) {
                    releaseLocalLock();
                }
            }

            return null;
        }

        private long reduceTimeout(long elapsedMillis) {
            return acquireMutexTimeout.toMillis() - elapsedMillis;
        }

        public void release() {
            runWithTryCatch(this::releaseLocks);
        }

        private Void releaseLocks() throws Exception {
            var stopWatch = Stopwatch.createStarted();
            try {
                releaseZkLock();
                log.info("Zk lock released within {} msec", stopAndGetElapsedMillis(stopWatch));
            } finally {
                stopWatch.reset().start();
                releaseLocalLock();
                log.info("Local lock released within {} msec", stopAndGetElapsedMillis(stopWatch));
            }
            return null;
        }

        @VisibleForTesting
        protected boolean acquireLocalLock() throws InterruptedException {
            return localLock.tryLock(acquireMutexTimeout.toMillis(), TimeUnit.MILLISECONDS);
        }

        @VisibleForTesting
        protected boolean acquireZkLock(long timeoutMillis) throws Exception {
            return zkLock.acquire(timeoutMillis, TimeUnit.MILLISECONDS);
        }

        @VisibleForTesting
        protected void releaseLocalLock() {
            localLock.unlock();
        }

        @VisibleForTesting
        protected void releaseZkLock() throws Exception {
            zkLock.release();
        }

        private static void runWithTryCatch(Callable<Void> callable) {
            try {
                callable.call();
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted", e);
            } catch (RuntimeException e) {
                throw e;
            } catch (Exception e) {
                throw new RuntimeException("Exception occured", e);
            }
        }

        private static long stopAndGetElapsedMillis(Stopwatch stopWatch) {
            return stopWatch.stop().elapsed(TimeUnit.MILLISECONDS);
        }

        @Override
        public void close() {
            release();
        }
    }

    @Value
    private static class ReleaseThreadLocalLock implements AutoCloseable {

        @Nonnull
        private static final ThreadLocal<Boolean> LOCK = ThreadLocal.withInitial(() -> false);

        @Nonnull
        CiProcessId processId;

        static boolean isAcquiredByCurrentThread() {
            return LOCK.get();
        }

        static ReleaseThreadLocalLock acquire(CiProcessId processId) {
            var lock = new ReleaseThreadLocalLock(processId);
            if (isRelease(processId)) {
                Preconditions.checkState(!isAcquiredByCurrentThread(), "Thread is already acquired lock");
                LOCK.set(true);
            }
            return lock;
        }

        void release() {
            if (isRelease(processId)) {
                Preconditions.checkState(isAcquiredByCurrentThread(), "Thread is already released lock");
                LOCK.set(false);
            }
        }

        @Override
        public void close() {
            release();
        }

        private static boolean isRelease(CiProcessId processId) {
            return processId.getType().isRelease();
        }
    }
}
