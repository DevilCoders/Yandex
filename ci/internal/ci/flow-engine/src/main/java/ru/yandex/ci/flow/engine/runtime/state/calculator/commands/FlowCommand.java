package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;

@Value
public class FlowCommand implements RecalcCommand {
    @Nonnull
    FlowLaunchId flowLaunchId;
    @Nullable
    FlowEvent event;
}
