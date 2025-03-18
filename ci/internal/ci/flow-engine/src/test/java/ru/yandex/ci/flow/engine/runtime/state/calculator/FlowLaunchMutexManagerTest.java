package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.nio.file.Path;
import java.time.Duration;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantLock;

import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.locks.InterProcessLock;
import org.apache.curator.framework.recipes.locks.InterProcessLockBasedOnReentrantLock;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

class FlowLaunchMutexManagerTest {

    private static final Duration TIMEOUT = Duration.ofSeconds(10);
    private static final CiProcessId RELEASE_PROCESS_ID = CiProcessId.ofRelease(Path.of("a.yaml"), "release-id");

    private static final LaunchId RELEASE_LAUNCH_ID_1 = LaunchId.of(RELEASE_PROCESS_ID, 1);
    private static final LaunchId RELEASE_LAUNCH_ID_2 = LaunchId.of(RELEASE_PROCESS_ID, 2);

    @Test
    @Timeout(10)
    public void releaseTriesToAcquireLockTwice() {
        var lockAcquiredCount = new AtomicInteger();
        var manager = createFlowLaunchMutexManager(lockAcquiredCount);

        manager.acquireAndRun(RELEASE_LAUNCH_ID_1, () -> {
            assertThat(lockAcquiredCount.get()).isEqualTo(1);
            manager.acquireAndRun(RELEASE_LAUNCH_ID_1, () -> {
                assertThat(lockAcquiredCount.get()).isEqualTo(1);
            });
        });
    }

    @Test
    @Timeout(10)
    public void releaseShouldNotTryToAcquireLockOfOtherRelease() {
        var lockAcquiredCount = new AtomicInteger();
        var manager = createFlowLaunchMutexManager(lockAcquiredCount);

        manager.acquireAndRun(RELEASE_LAUNCH_ID_1, () -> {
            assertThat(lockAcquiredCount.get()).isEqualTo(1);
            manager.acquireAndRun(RELEASE_LAUNCH_ID_2, () -> {
                assertThat(lockAcquiredCount.get()).isEqualTo(1);
            });
        });
    }

    @Test
    @Timeout(10)
    public void twoThreadsAcquiresLocksForDifferentReleases() throws Exception {
        var lockAcquiredCount = new AtomicInteger();
        var manager = createFlowLaunchMutexManager(lockAcquiredCount);

        var acquiredLock1 = new CountDownLatch(1);
        var releasedLock1 = new CountDownLatch(1);
        var thread1 = new Thread(() ->
                manager.acquireAndRun(RELEASE_LAUNCH_ID_1, () -> {
                    acquiredLock1.countDown();
                    await(releasedLock1);
                })
        );
        thread1.start();

        var acquiredLock2 = new CountDownLatch(1);
        var releasedLock2 = new CountDownLatch(1);
        var thread2 = new Thread(() ->
                manager.acquireAndRun(RELEASE_LAUNCH_ID_2, () -> {
                    acquiredLock2.countDown();
                    await(releasedLock2);
                })
        );
        thread2.start();

        await(acquiredLock1);
        await(acquiredLock2);

        releasedLock1.countDown();
        releasedLock2.countDown();

        thread1.join();
        thread2.join();
    }

    private static FlowLaunchMutexManager createFlowLaunchMutexManager(AtomicInteger lockAcquiredCount) {
        return new FlowLaunchMutexManager(mock(CuratorFramework.class), 512, 512, TIMEOUT, Duration.ofHours(1)) {

            @Override
            protected InterProcessLock createZkLock(String zkPath) {
                return new InterProcessLockBasedOnReentrantLock(new ReentrantLock());
            }

            @Override
            protected Mutex createMutex(String zkPath, ReentrantLock localLock, InterProcessLock zkLock) {
                return new Mutex(zkPath, localLock, zkLock) {
                    @Override
                    protected boolean acquireLocalLock() throws InterruptedException {
                        lockAcquiredCount.incrementAndGet();
                        return super.acquireLocalLock();
                    }

                    @Override
                    protected void releaseLocalLock() {
                        lockAcquiredCount.decrementAndGet();
                        super.releaseLocalLock();
                    }
                };
            }
        };
    }

    private static void await(CountDownLatch countDownLatch) {
        try {
            countDownLatch.await();
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted", e);
        }
    }

}
