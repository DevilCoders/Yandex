package org.apache.curator.framework.recipes.locks;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;

public class InterProcessLockBasedOnReentrantLock implements InterProcessLock {

    private final ReentrantLock lock;

    public InterProcessLockBasedOnReentrantLock(ReentrantLock lock) {
        this.lock = lock;
    }

    @Override
    public void acquire() throws Exception {
        lock.lockInterruptibly();
    }

    @Override
    public boolean acquire(long time, TimeUnit unit) throws Exception {
        return lock.tryLock(time, unit);
    }

    @Override
    public void release() throws Exception {
        lock.unlock();
    }

    @Override
    public boolean isAcquiredInThisProcess() {
        return lock.isLocked();
    }
}
