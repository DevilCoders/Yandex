package ru.yandex.ci.engine.notification.xiva;

import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.ToString;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;

@ToString
public class ReleasesTimelineChangedEvent extends XivaBaseEvent {

    public ReleasesTimelineChangedEvent(CiProcessId processId, List<String> branches) {
        super(
                "releases-timeline@" + processId.getDir() + "@" + processId.getSubId(),
                branches
        );
    }

    @Override
    public Type getType() {
        return Type.RELEASES_TIMELINE_CHANGED;
    }

    static Optional<ReleasesTimelineChangedEvent> onLaunchStateChanged(@Nonnull Launch updatedLaunch,
                                                                       @Nullable Launch oldLaunch) {
        if (isTriggered(updatedLaunch, oldLaunch)) {
            return Optional.of(new ReleasesTimelineChangedEvent(
                    updatedLaunch.getProcessId(),
                    getAffectedBranches(updatedLaunch)
            ));
        }
        return Optional.empty();
    }

    private static List<String> getAffectedBranches(Launch updatedLaunch) {
        var vcsInfo = updatedLaunch.getVcsInfo();
        return Stream.of(vcsInfo.getRevision().getBranch(), vcsInfo.getSelectedBranch())
                .filter(Objects::nonNull)
                .map(ArcBranch::getBranch)
                .collect(Collectors.toList());
    }

    private static boolean isTriggered(@Nonnull Launch updatedLaunch,
                                       @Nullable Launch oldLaunch) {
        if (!updatedLaunch.getProcessId().getType().isRelease()) {
            return false;
        }
        return oldLaunch == null || updatedLaunch.getStatus() != oldLaunch.getStatus();
    }

}
