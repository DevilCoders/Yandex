package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;

public class ScheduleCommand implements PendingCommand {
    private final FullJobLaunchId jobLaunchId;

    public ScheduleCommand(FullJobLaunchId jobLaunchId) {
        this.jobLaunchId = jobLaunchId;
    }

    public FullJobLaunchId getJobLaunchId() {
        return jobLaunchId;
    }

}
