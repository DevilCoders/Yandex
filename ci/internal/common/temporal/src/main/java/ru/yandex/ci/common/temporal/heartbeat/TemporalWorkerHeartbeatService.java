package ru.yandex.ci.common.temporal.heartbeat;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.common.util.concurrent.AbstractScheduledService;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import io.temporal.activity.ActivityExecutionContext;
import io.temporal.client.ActivityCompletionException;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.util.HostnameUtils;

/**
 * Темпорал не предусматривает какого-либо worker-wide механизма про проверки живости воркета
 * Вместо этого он предполагает использовать механизм Heartbeat'ов отдельно по каждому Activity
 * https://docs.temporal.io/docs/concepts/what-is-an-activity-heartbeat/
 * <p>
 * Этот сервис предназначен для того, что бы регулярно отправлять Heartbeat'ы по всем активити этого воркера
 * <p>
 * Implementation nodes:
 * 1. Запрос на heartbeat Temporal может отправлять как синхронно, так и асинхронно,
 * если решает что текущий heartbeat достаточно свежий
 * 2. Исходя из п.1 можно оправлять heartbeat'ы достаточно часто
 * 3. Отдельный executorService с достаточно большим количеством потоков используется на случай,
 * если понадобиться отправить много синхронных heartbeat'ов.
 * 4. Сама отправка heartbeat'ов асинхронно ретраятся Temporal'ом
 */
@Slf4j
public class TemporalWorkerHeartbeatService extends AbstractScheduledService {

    private final String hostname;
    private final Duration heartbeatInterval;
    private final ExecutorService executorService;

    private final ConcurrentMap<String, ExecutingActivity> activities = new ConcurrentHashMap<>();

    private final AtomicInteger lastIterationActivities = new AtomicInteger();
    private final AtomicInteger lastIterationFailedHeartbeats = new AtomicInteger();

    private static final String METRIC_NAME = "temporal_activity_heartbeats";
    private static final String TAG_NAME = "counter";

    public TemporalWorkerHeartbeatService(Duration heartbeatInterval, int threadCount, MeterRegistry meterRegistry) {
        this(heartbeatInterval, threadCount, meterRegistry, HostnameUtils.getHostname());
    }

    public TemporalWorkerHeartbeatService(Duration heartbeatInterval, int threadCount,
                                          MeterRegistry meterRegistry, String hostname) {
        this.hostname = hostname;
        this.heartbeatInterval = heartbeatInterval;
        this.executorService = Executors.newFixedThreadPool(
                threadCount,
                new ThreadFactoryBuilder()
                        .setDaemon(true)
                        .setNameFormat("temporal-activity-heartbeat-%d")
                        .build()
        );
        createMetrics(meterRegistry);
    }

    private void createMetrics(MeterRegistry meterRegistry) {
        Gauge.builder(METRIC_NAME, lastIterationActivities::get)
                .tag(TAG_NAME, "current_activities")
                .register(meterRegistry);

        Gauge.builder(METRIC_NAME, lastIterationFailedHeartbeats::get)
                .tag(TAG_NAME, "failed_heartbeats")
                .register(meterRegistry);
    }

    @Override
    protected void runOneIteration() throws InterruptedException {
        List<ExecutingActivity> currentActivities = new ArrayList<>(this.activities.values());
        log.info("Running heartbeat iteration for {} activities", currentActivities.size());

        AtomicInteger failedCounter = new AtomicInteger();

        try {
            List<Future<?>> futures = new ArrayList<>();
            for (ExecutingActivity activity : currentActivities) {
                futures.add(executorService.submit(() -> heartbeat(activity, failedCounter)));
            }
            for (Future<?> future : futures) {
                future.get();
            }
            log.info("Heartbeat iteration finished");
            failedCounter.set(failedCounter.get());
        } catch (ExecutionException e) {
            log.error("Exception while processing heartbeat iteration", e);
            failedCounter.set(currentActivities.size());
        }

        lastIterationActivities.set(currentActivities.size());
        lastIterationFailedHeartbeats.set(failedCounter.get());

    }

    private void heartbeat(ExecutingActivity activity, AtomicInteger failedCounter) {
        try {
            activity.context.heartbeat(hostname);
        } catch (ActivityCompletionException e) {
            failedCounter.incrementAndGet();
            log.warn("Got ActivityCompletionException for {}, interrupting it.", activity, e);
            interruptActivity(activity.getId());
        } catch (Exception e) {
            failedCounter.incrementAndGet();
            log.warn("Failed to send heartbeat for {}", activity);
        }
    }

    private void interruptActivity(String activityId) {
        //Explicitly get activity with lock so, that we don't interrupt another thread due to a race
        activities.compute(activityId, (s, currentActivity) -> {
            if (currentActivity == null) {
                log.info("Activity already unregistered in service: {}", activityId);
            } else {
                log.info("Interrupting thread {}", currentActivity.thread.getName());
                currentActivity.thread.interrupt();
            }
            return currentActivity;
        });
    }

    @Override
    protected Scheduler scheduler() {
        return Scheduler.newFixedDelaySchedule(heartbeatInterval, heartbeatInterval);
    }

    public void registerActivity(ActivityExecutionContext context, Thread thread) {
        var activity = new ExecutingActivity(context, thread);
        log.info("Staring heartbeats for {}", activity);
        activities.put(activity.getId(), new ExecutingActivity(context, thread));
    }

    public void unregisterActivity(ActivityExecutionContext context) {
        var activity = activities.remove(context.getInfo().getActivityId());
        if (activity != null) {
            log.info("Stopping heartbeats for {}", activity);
        } else {
            log.warn("Trying to remove not existing activity {}: ", context.getInfo().getActivityId());
        }
    }

    @Value
    private static class ExecutingActivity {

        ActivityExecutionContext context;
        Thread thread;

        private String getId() {
            return context.getInfo().getActivityId();
        }

        @Override
        public String toString() {
            var info = context.getInfo();
            return String.format(
                    "Workflow '%s', activity '%s', attempt '%s', runId '%s', thread '%s'",
                    info.getWorkflowId(),
                    info.getActivityId(),
                    info.getAttempt(),
                    info.getRunId(),
                    thread.getName()
            );
        }
    }
}
