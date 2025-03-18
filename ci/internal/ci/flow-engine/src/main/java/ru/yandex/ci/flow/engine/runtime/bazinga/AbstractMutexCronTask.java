package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.google.common.base.Stopwatch;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.locks.InterProcessSemaphoreMutex;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

/**
 * CRON задачи с блокировкой в ZK, гарантирующая выполнение задачи в едином экземпляре
 */
public abstract class AbstractMutexCronTask extends CronTask {
    protected final Logger log = LoggerFactory.getLogger(getClass());

    @Nullable
    private final InterProcessSemaphoreMutex mutex;

    protected AbstractMutexCronTask(@Nullable CuratorFramework curator) {
        this.mutex = curator != null ? new InterProcessSemaphoreMutex(curator, mutexPath()) : null;
    }

    @Override
    public final void execute(ExecutionContext executionContext) throws Exception {
        if (mutex == null) {
            this.executeImpl(executionContext);
        } else {
            var stopWatch = Stopwatch.createStarted();
            var timeout = timeout();
            if (!mutex.acquire(timeout.getMillis(), TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Cannot acquire Cron task lease within " +
                        timeout.getStandardSeconds() + " seconds");
            }
            log.info("Mutex acquired within {} msec", stopWatch.stop().elapsed(TimeUnit.MILLISECONDS));
            try {
                this.executeImpl(executionContext);
            } finally {
                stopWatch.reset().start();
                mutex.release();
                log.info("Mutex released within {} msec", stopWatch.stop().elapsed(TimeUnit.MILLISECONDS));
            }
        }
    }

    protected abstract void executeImpl(ExecutionContext executionContext) throws Exception;

    private String mutexPath() {
        return "/cron_mutex/" + id().getId();
    }
}
