package ru.yandex.ci.engine.autocheck.model;

import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.engine.autocheck.AutocheckConfiguration;
import ru.yandex.ci.storage.core.CheckOuterClass;

@Value
@Builder
@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
public class CheckRegistrationParams {
    @Nonnull
    CheckLaunchParams launchParams;

    @Nonnull
    AutocheckLaunchConfig autocheckLaunchConfig;

    @Nonnull
    List<AutocheckConfiguration> autocheckConfigurations;

    @Nonnull
    Set<String> disabledConfigurations;

    @Nonnull
    CheckOuterClass.DistbuildPriority distbuildPriority;

    @Nonnull
    CheckOuterClass.Zipatch zipatch;

    public ArcRevision getAutocheckConfigLeftRevision() {
        return autocheckLaunchConfig.getLeftConfigBundle().getRevision();
    }

    public ArcRevision getAutocheckConfigRightRevision() {
        return autocheckLaunchConfig.getRightConfigBundle().getRevision();
    }

}
