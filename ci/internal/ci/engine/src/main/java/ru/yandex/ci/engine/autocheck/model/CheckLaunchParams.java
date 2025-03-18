package ru.yandex.ci.engine.autocheck.model;

import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;

@Value
@Builder
public class CheckLaunchParams {

    @Nonnull
    String launchId;

    @Nonnull
    CiProcessId ciProcessId;

    int launchNumber;

    @Nonnull
    String checkAuthor;

    @Nonnull
    OrderedArcRevision leftRevision;

    @Nonnull
    OrderedArcRevision rightRevision;

    @Nonnull
    String arcanumCheckId;

    @Nullable
    Long pullRequestId;

    @Nullable
    Long diffSetId;

    @Nonnull
    String gsidBase;

    @Nullable
    Instant diffSetEventCreated;

    boolean precommitCheck;

    boolean stressTest;

    boolean spRegistrationDisabled;
}
