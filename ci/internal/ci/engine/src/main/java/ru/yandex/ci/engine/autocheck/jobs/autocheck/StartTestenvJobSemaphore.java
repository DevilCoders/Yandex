package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import com.google.common.base.Stopwatch;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.locks.InterProcessSemaphoreV2;
import org.apache.curator.framework.recipes.locks.Lease;

import ru.yandex.ci.core.db.CiMainDb;

@Slf4j
public class StartTestenvJobSemaphore {

    @Nonnull
    private final InterProcessSemaphoreV2 zkSemaphore;

    public StartTestenvJobSemaphore(CuratorFramework curatorFramework, CiMainDb db) {
        zkSemaphore = new InterProcessSemaphoreV2(curatorFramework, "/autocheck/start_te_jobs",
                getSemaphoreLimit(db));
    }

    public Semaphore acquire() throws Exception {
        var finalZkSemaphore = zkSemaphore;

        log.info("Zk lock acquiring");
        var stopWatch = Stopwatch.createStarted();
        var semaphore = new Semaphore(finalZkSemaphore, finalZkSemaphore.acquire());
        log.info("Zk lock acquired within {} msec", stopWatch.elapsed(TimeUnit.MILLISECONDS));

        return semaphore;
    }

    private static int getSemaphoreLimit(CiMainDb db) {
        return db.currentOrReadOnly(() -> db.keyValue().getInt(
                "StartTestenvJobSemaphore", "limit", 5
        ));
    }

    @Value
    public static class Semaphore implements AutoCloseable {

        @Nonnull
        InterProcessSemaphoreV2 semaphore;
        @Nonnull
        Lease lease;

        @Override
        public void close() throws Exception {
            var stopWatch = Stopwatch.createStarted();
            semaphore.returnLease(lease);
            log.info("Zk lock released within {} msec", stopWatch.elapsed(TimeUnit.MILLISECONDS));
        }
    }


}
