package ru.yandex.ci.engine.launch.auto;

import java.time.Instant;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.TaskCategory;

@Slf4j
@RequiredArgsConstructor
public class ReleaseScheduler {
    private final BazingaTaskManager bazingaTaskManager;

    public void schedule(CiProcessId processId, Instant scheduledAt) {
        var task = new DelayedAutoReleaseLaunchTask(processId);
        var jodaInstant = new org.joda.time.Instant(scheduledAt.toEpochMilli());

        bazingaTaskManager.schedule(task,
                TaskCategory.DEFAULT, jodaInstant, task.priority(), false,
                Option.of(taskGroup(processId))
        );
        log.info("Scheduled launch release {} at {}", processId, scheduledAt);
    }

    private static String taskGroup(CiProcessId processId) {
        return "scheduledRelease:" + processId.asString();
    }
}
