package ru.yandex.ci.api.controllers.frontend;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;

@Value
@Builder
public class ForceSuccessResult {
    @Nonnull
    FrontendFlowApi.LaunchState launchState;

    @Nullable
    String message;
}
