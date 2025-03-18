package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.flow.engine.definition.job.InterruptMethod;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;

@Value
public class InterruptJobCommand implements PendingCommand {
    @Nonnull
    FullJobLaunchId jobLaunchId;
    @Nonnull
    InterruptMethod interruptMethod;
}
