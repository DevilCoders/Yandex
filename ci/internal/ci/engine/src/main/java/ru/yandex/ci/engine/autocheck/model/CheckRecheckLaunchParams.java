package ru.yandex.ci.engine.autocheck.model;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;

@Value
@Builder
public class CheckRecheckLaunchParams {
    @Nonnull
    CheckOuterClass.Check check;

    @Nonnull
    CheckIteration.Iteration iteration;

    @Nonnull
    String gsidBase;

    @Nonnull
    List<StorageApi.SuiteRestart> suites;
}
