package ru.yandex.ci.core.timeline;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder
public class TimelineItem {
    CiProcessId processId;
    /**
     * Ревизия, порядковый номер коммита.
     */
    long revision;
    /**
     * Номер элемента, на одной ревизии может быть несколько item.
     */
    int number;

    @Nullable
    Launch launch;

    @Nullable
    Branch branch;

    @Nullable
    @With
    String showInBranch;

    boolean inStable;

    public TimelineItem(CiProcessId processId, long revision, int number,
                        @Nullable Launch launch,
                        @Nullable Branch branch,
                        @Nullable String showInBranch,
                        boolean inStable) {
        this.processId = processId;
        this.revision = revision;
        this.number = number;
        this.launch = launch;
        this.branch = branch;
        this.showInBranch = showInBranch;
        this.inStable = inStable;

        Preconditions.checkState(launch != null || branch != null,
                "Either launch or branch must be not null, but both are null");
        Preconditions.checkArgument(getArcRevision().getNumber() == revision,
                "revision doesn't equal launch or branch revision; timeline: %s (number: %s, type: %s), item: %s",
                revision, number, getType(), getArcRevision().getNumber());
    }

    public Offset getNextStart() {
        return Offset.of(revision, number);
    }

    @SuppressWarnings("ConstantConditions")
    public OrderedArcRevision getArcRevision() {
        return switch (getType()) {
            case LAUNCH -> launch.getVcsInfo().getRevision();
            case BRANCH -> branch.getInfo().getBaseRevision();
        };
    }

    public Type getType() {
        return launch != null
                ? Type.LAUNCH
                : Type.BRANCH;
    }

    public enum Type {
        BRANCH,
        LAUNCH
    }
}


