package ru.yandex.ci.engine.timeline;

import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.timeline.ReleaseCommit;

/**
 * см. service.proto/TimelineCommit
 */
@Value
public class TimelineCommit {
    ReleaseCommit commit;
    @Nullable
    Launch release;

    public static TimelineCommit forSingleActionCommit(ArcCommit commit, OrderedArcRevision revision) {
        return new TimelineCommit(new ReleaseCommit(commit, revision, List.of(), Set.of(), null), null);
    }
}
