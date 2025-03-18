package ru.yandex.ci.flow.engine.runtime;

import java.nio.charset.StandardCharsets;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.InterruptMethod;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.ProducedResourcesValidationException;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.exceptions.JobManualFailException;
import ru.yandex.ci.flow.zookeeper.CuratorFactory;
import ru.yandex.ci.flow.zookeeper.CuratorValueObservable;

@Slf4j
public class SyncJobExecutor implements AutoCloseable {

    private final JobExecutor executor;
    private final JobContext jobContext;
    private final Runnable onInterruptFailed;
    private final boolean recover;
    private final Thread currentThread;

    private final CuratorValueObservable curatorObservable;
    private final String nodePath;

    private StoredResourceContainer producedResources;

    @Nullable
    private volatile Throwable executorException;

    private State state = State.IDLE;

    public SyncJobExecutor(
            @Nonnull CuratorFactory curatorFactory,
            @Nonnull JobExecutor executor,
            @Nonnull JobContext jobContext,
            @Nonnull Runnable onInterruptFailed,
            boolean recover
    ) {
        this.executor = executor;
        this.jobContext = jobContext;
        this.onInterruptFailed = onInterruptFailed;
        this.recover = recover;

        this.producedResources = StoredResourceContainer.empty();
        this.currentThread = Thread.currentThread();

        this.nodePath = getInterruptNodePath(jobContext.getFullJobId());
        log.info("Job curator node: {}", nodePath);

        this.curatorObservable = curatorFactory.createValueObservable(nodePath);
    }

    public void execute() {
        Preconditions.checkState(Thread.currentThread() == currentThread,
                "Internal error. Cannot migrate between threads");

        synchronized (this) {
            Preconditions.checkState(state == State.IDLE,
                    "Current state must be %s, but %s", State.IDLE, state);
        }

        log.info("SyncJobExecutor#start");

        curatorObservable.observe(this::valueChanged);
        curatorObservable.start();

        this.runSync();
    }

    private void runSync() {
        var fullJobId = jobContext.getFullJobId();
        try {
            synchronized (this) {
                if (state == State.INTERRUPTED || state == State.KILLED) {
                    return; // ---
                }
                state = State.RUNNING;
            }
            if (!recover) {
                log.info("Starting job {}", fullJobId);
            } else {
                log.info("Recovering job {}", fullJobId);
            }

            executor.execute(jobContext);

            producedResources = jobContext.resources().getStoredProducedResources();

            log.info("Job execution finished {}", fullJobId);
        } catch (OutOfMemoryError e) {
            throw e; //Don't try to catch OOM
        } catch (Throwable e) {
            synchronized (this) {
                if (state == State.RUNNING) {
                    state = State.IDLE;
                    executorException = e;
                }
            }

            if (e instanceof JobManualFailException) {
                jobContext.progress().updateText(e.getMessage());
            }

            // Если джоба смогла произвести правильные ресурсы перед падением, сохраняем их.
            // Это нужно для метода JobContext.getAllProducedResourcesFromAllLaunchesUnsafe и очистки мультитестингов.
            // Подробнее в st/MARKETINFRA-2823.
            try {
                producedResources = jobContext.resources().getStoredProducedResources();
            } catch (ProducedResourcesValidationException ignored) {
                // Сюда мы попадаем если валидация произведённых ресурсов упала, то есть их слишком много или слишком
                // мало. Это ок.
            }
        } finally {
            synchronized (this) {
                if (state == State.RUNNING) {
                    state = State.IDLE;
                }
            }
        }
    }

    public StoredResourceContainer getProducedResources() {
        return producedResources;
    }

    @Nullable
    public Throwable getExecutorException() {
        return executorException;
    }

    private void valueChanged(byte[] data) {
        String message = new String(data, StandardCharsets.UTF_8);
        if (message.equals(InterruptMethod.INTERRUPT.name())) {
            log.info("Got interrupted notification by node: {}", nodePath);
            try {
                executor.interrupt(jobContext);
            } catch (Exception e) {
                log.error("Unable to interrupt a job", e);
                onInterruptFailed.run();
                return; // ---
            }
            synchronized (this) {
                boolean interrupt = state == State.RUNNING;
                state = State.INTERRUPTED;
                if (interrupt) {
                    currentThread.interrupt();
                }
            }
        } else if (message.equals(InterruptMethod.KILL.name())) {
            log.info("Got kill notification by node: {}", nodePath);
            try {
                executor.interrupt(jobContext);
            } catch (Exception e) {
                log.error("Unable to kill a job", e);
            } finally {
                synchronized (this) {
                    boolean interrupt = state == State.RUNNING;
                    state = State.KILLED;
                    if (interrupt) {
                        currentThread.interrupt();
                    }
                }
            }
        } else {
            log.error("Unknown interrupt message: '{}'", message);
        }
    }

    public State getState() {
        return state;
    }

    @Override
    public void close() throws Exception {
        log.info("JobExecutor#close");
        curatorObservable.close();
    }

    public static String getInterruptNodePath(String fullJobId) {
        return String.format("/job_interrupt/%s", fullJobId);
    }

    enum State {
        IDLE, RUNNING, INTERRUPTED, KILLED
    }
}
