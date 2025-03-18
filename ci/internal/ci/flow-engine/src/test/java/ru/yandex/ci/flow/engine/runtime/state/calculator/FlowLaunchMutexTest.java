package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.time.Duration;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.locks.InterProcessLockBasedOnReentrantLock;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

class FlowLaunchMutexTest {

    private static final Duration TIMEOUT = Duration.ofSeconds(10);

    @Test
    @Timeout(10)
    public void lockAndUnlockOrder() throws Exception {
        /* Порядок захвата и освобождения локов:
            - захватываем сначала локальный, затем zk
            - освобождаем сначала zk, затем локальный
           Такой порядок даёт возможность другому серверу захватить zk-лок и начать выполнять работу */
        var manager = new FlowLaunchMutexManager(mock(CuratorFramework.class), 512, 512, TIMEOUT, Duration.ofHours(1));
        var localLock = new ReentrantLock();
        var zkLock = new ReentrantLock();

        // через эти локи разрешаем захват определённого лока (localLock или zkLock)
        var allowAcquireLocalLock = new CountDownLatch(1);
        var allowAcquireZkLock = new CountDownLatch(1);
        // эти локи нужны для сигнала о захвате лока
        var localLockAcquired = new CountDownLatch(1);
        var zkLockAcquired = new CountDownLatch(1);
        // через эти локи разрешаем разблокировку лока
        var allowReleaseLocalLock = new CountDownLatch(1);
        var allowReleaseZkLock = new CountDownLatch(1);

        var mutex = manager.new Mutex("zkPath", localLock, new InterProcessLockBasedOnReentrantLock(zkLock)) {
            @Override
            protected boolean acquireLocalLock() throws InterruptedException {
                await(allowAcquireLocalLock);
                var result = super.acquireLocalLock();
                localLockAcquired.countDown();
                return result;
            }

            @Override
            protected boolean acquireZkLock(long timeoutMillis) throws Exception {
                await(allowAcquireZkLock);
                var result = super.acquireZkLock(timeoutMillis);
                zkLockAcquired.countDown();
                return result;
            }

            @Override
            protected void releaseLocalLock() {
                await(allowReleaseLocalLock);
                super.releaseLocalLock();
            }

            @Override
            protected void releaseZkLock() throws Exception {
                await(allowReleaseZkLock);
                super.releaseZkLock();
            }
        };

        // создаём тред, который захватывает local и zk lock
        var anotherThreadAcquiredMutex = new CountDownLatch(1);
        var thread = new Thread(() -> {
            mutex.acquireOrThrow();
            try (var acquiredMutex = mutex) {
                anotherThreadAcquiredMutex.countDown();
            }
        });
        thread.start();

        // разрешаем захватить local lock
        allowAcquireLocalLock.countDown();
        await(localLockAcquired);
        assertThat(localLock.isLocked() && !localLock.isHeldByCurrentThread()).isTrue();
        assertThat(!zkLock.isLocked()).isTrue();

        // разрешаем захватить zk lock
        allowAcquireZkLock.countDown();
        await(zkLockAcquired);
        assertThat(localLock.isLocked() && !localLock.isHeldByCurrentThread()).isTrue();
        assertThat(zkLock.isLocked() && !zkLock.isHeldByCurrentThread()).isTrue();

        // ждём, когда тред захватит local и zk lock
        anotherThreadAcquiredMutex.await();
        assertThat(localLock.isLocked() && !localLock.isHeldByCurrentThread()).isTrue();
        assertThat(zkLock.isLocked() && !zkLock.isHeldByCurrentThread()).isTrue();

        // разрешаем отпустить zk lock и пытаемся сами его захватить
        allowReleaseZkLock.countDown();
        zkLock.lockInterruptibly();

        assertThat(localLock.isLocked() && !localLock.isHeldByCurrentThread()).isTrue();
        assertThat(zkLock.isLocked() && zkLock.isHeldByCurrentThread()).isTrue();

        // разрешаем отпустить local lock и пытаемся сами его захватить
        allowReleaseLocalLock.countDown();
        localLock.lockInterruptibly();
        assertThat(localLock.isLocked() && localLock.isHeldByCurrentThread()).isTrue();
        assertThat(zkLock.isLocked() && zkLock.isHeldByCurrentThread()).isTrue();
    }

    private static void await(CountDownLatch countDownLatch) {
        try {
            countDownLatch.await();
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted", e);
        }
    }

}
