package ru.yandex.ci.engine.launch.version;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.versioning.Version;

@RequiredArgsConstructor
public class BranchVersionService {

    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final VersionSlotService versionSlotService;

    public Version nextBranchVersion(CiProcessId processId, OrderedArcRevision revision,
                                     @Nullable Integer startVersion) {
        ArcBranch baseBranch = revision.getBranch();
        Preconditions.checkArgument(baseBranch.isTrunk(), "ветки от веток не поддержаны");

        return versionSlotService.nextVersion(SlotFor.BRANCH, processId, revision,
                () -> VersionUtils.next(db, processId, baseBranch, startVersion)
        );
    }

}
