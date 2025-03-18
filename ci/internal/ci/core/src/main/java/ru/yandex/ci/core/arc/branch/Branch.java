package ru.yandex.ci.core.arc.branch;

import javax.annotation.Nonnull;

import lombok.Value;
import lombok.With;
import lombok.experimental.Delegate;

import ru.yandex.ci.core.timeline.TimelineBranchItem;

@Value(staticConstructor = "of")
@Nonnull
@SuppressWarnings("ReferenceEquality")
public class Branch {
    @With
    BranchInfo info;

    @Delegate
    @With
    TimelineBranchItem item;
}
