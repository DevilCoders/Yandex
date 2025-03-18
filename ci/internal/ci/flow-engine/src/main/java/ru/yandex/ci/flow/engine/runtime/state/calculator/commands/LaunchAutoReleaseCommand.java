package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

@Value
public class LaunchAutoReleaseCommand implements PendingCommand {

    @Nonnull
    FlowLaunchEntity flowLaunch;

}
