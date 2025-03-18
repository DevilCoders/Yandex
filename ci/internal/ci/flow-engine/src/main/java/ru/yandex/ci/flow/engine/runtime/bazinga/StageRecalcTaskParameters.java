package ru.yandex.ci.flow.engine.runtime.bazinga;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@BenderBindAllFields
@Value
public class StageRecalcTaskParameters {

    @Nonnull
    FlowLaunchId flowLaunchId;
}
