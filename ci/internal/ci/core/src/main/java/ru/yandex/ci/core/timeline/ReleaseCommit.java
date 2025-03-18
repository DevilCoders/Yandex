package ru.yandex.ci.core.timeline;

import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Launch;

@Value
public class ReleaseCommit {
    ArcCommit commit;
    OrderedArcRevision revision;
    List<Launch> cancelledReleases;
    Set<ArcBranch> branches;
    @Nullable
    DiscoveredCommitState discoveredState;
}
