package ru.yandex.ci.engine.launch.version;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.engine.branch.BranchService;

@Slf4j
@RequiredArgsConstructor
public class LaunchVersionService {

    @Nonnull
    private final BranchService branchService;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final VersionSlotService versionSlotService;

    public Version nextLaunchVersion(CiProcessId processId,
                                     OrderedArcRevision revision,
                                     ArcBranch selectedBranch,
                                     @Nullable Integer startVersion) {
        log.info("Getting next launch version for {} at {}/{}", processId, revision, selectedBranch);
        return versionSlotService.nextVersion(SlotFor.RELEASE, processId, revision,
                () -> generateNextVersion(processId, selectedBranch, startVersion));
    }

    private Version generateNextVersion(CiProcessId processId, ArcBranch branch, @Nullable Integer startVersion) {

        if (!branch.isTrunk()) {
            log.info("Generating next version in branch {}", branch);
            Branch branchInfo = branchService.getBranch(branch, processId);

            Version branchVersion = branchInfo.getVersion();
            Preconditions.checkState(
                    branchVersion.getMinor() == null, "branch contains minor version %s",
                    branchVersion
            );

            String nextInBranch = String.valueOf(db.counter().incrementAndGet(
                    VersionUtils.VERSION_NAMESPACE,
                    VersionUtils.counterKey(processId, branch)
            ));
            log.info("Branch version: {}, next in branch: {}", branchVersion, nextInBranch);

            OrderedArcRevision baseRevision = branchInfo.getInfo().getBaseRevision();
            log.info("Reserve slot at base revision {} for version {}. It couldn't be used anymore",
                    baseRevision, branchVersion);
            versionSlotService.occupySlot(SlotFor.RELEASE, processId, baseRevision, branchVersion);

            Version version = Version.majorMinor(branchVersion.getMajor(), nextInBranch);
            log.info("Generated version {}", version);
            return version;
        }

        log.info("Generating next major version in trunk");
        return VersionUtils.next(db, processId, branch, startVersion);
    }

    public Version genericVersion(LaunchId launchId) {
        return Version.major(Integer.toString(launchId.getNumber()));
    }
}
