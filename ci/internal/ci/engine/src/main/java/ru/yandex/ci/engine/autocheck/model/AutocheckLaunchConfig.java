package ru.yandex.ci.engine.autocheck.model;

import java.util.NavigableSet;
import java.util.Set;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.engine.autocheck.config.AutocheckYamlService;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;

@Value
@AllArgsConstructor
public class AutocheckLaunchConfig {

    @Nonnull
    Set<CheckOuterClass.NativeBuild> nativeBuilds;

    @Nonnull
    Set<CheckOuterClass.LargeAutostart> largeTests;

    @Nonnull
    CheckOuterClass.LargeConfig largeConfig;

    @Nonnull
    NavigableSet<String> fullTargets;

    @Nonnull
    NavigableSet<String> fastTargets;

    boolean sequentialMode;

    @Nonnull
    Set<String> invalidAYamls;

    @Nonnull
    String poolName;

    boolean autodetectedFastCircuit;

    @Nonnull
    CheckIteration.StrongModePolicy strongModePolicy;

    @Nonnull
    AutocheckYamlService.ConfigBundle leftConfigBundle;

    @Nonnull
    AutocheckYamlService.ConfigBundle rightConfigBundle;

    public boolean hasFastCircuit() {
        return !fastTargets.isEmpty();
    }

}
