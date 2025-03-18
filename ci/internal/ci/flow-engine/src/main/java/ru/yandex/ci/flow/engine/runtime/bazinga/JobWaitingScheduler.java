package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Instant;

import javax.annotation.Nullable;

import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.WaitingForScheduleCommand;

public interface JobWaitingScheduler {
    void schedule(FullJobLaunchId fullJobLaunchId, @Nullable Instant date);

    void retry(JobScheduleTask jobScheduleTask, Instant date);

    default void schedule(WaitingForScheduleCommand scheduleCommand) {
        schedule(scheduleCommand.getJobLaunchId(), scheduleCommand.getDate());
    }
}
