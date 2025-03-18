package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;

@Value
public class WaitingForScheduleCommand implements PendingCommand {
    @Nonnull
    FullJobLaunchId jobLaunchId;

    @Nullable
    Instant date;
}
