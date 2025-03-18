package ru.yandex.ci.engine.branch;

import java.time.Clock;
import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimaps;
import com.google.common.collect.SetMultimap;
import io.grpc.StatusRuntimeException;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.arc.branch.BranchInfoByCommitId;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.timeline.BranchState;
import ru.yandex.ci.core.timeline.BranchVcsInfo;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.core.timeline.TimelineBranchItemByUpdateDate;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.launch.version.BranchVersionService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.engine.timeline.TimelineService;

@Slf4j
@RequiredArgsConstructor
public class BranchService {
    private static final int CONFLICT_RESOLUTION_MAX_TRIES = 10;

    @Nonnull
    private final Clock clock;
    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final TimelineService timelineService;
    @Nonnull
    private final ConfigurationService configurationService;
    @Nonnull
    private final BranchVersionService branchVersionService;
    @Nonnull
    private final SecurityAccessService securityAccessService;
    @Nonnull
    private final CommitRangeService commitRangeService;
    @Nonnull
    private final BranchNameGenerator nameGenerator;

    public Branch createBranch(CiProcessId processId, OrderedArcRevision revision, String login) {
        log.info("#createBranch, processId = {}, revision  {}, login = {}",
                processId, revision, login);

        ArcBranch baseBranch = revision.getBranch();
        Preconditions.checkArgument(baseBranch.isTrunk(),
                "cannot create branch from another branch revision %s", revision);

        ConfigBundle configBundle = configurationService.getLastValidConfig(processId.getPath(), baseBranch);

        ReleaseConfig releaseConfig = configBundle.getValidReleaseConfigOrThrow(processId);
        Version version = branchVersionService.nextBranchVersion(processId, revision, releaseConfig.getStartVersion());

        Branch branch = findImplicit(revision.toRevision(), processId, version)
                .orElseGet(() -> createBranchWithAvailableName(
                        configBundle,
                        processId,
                        revision,
                        login,
                        version)
                );

        log.info("Branch created: {}", branch);
        timelineService.addBranch(branch);

        return branch;
    }

    /**
     * Создание ветки, без отображения ее в Timeline.
     * <p>
     * При последующем создании ветки на ревизии, будет использована неявная ветки, если она есть.
     */
    public Branch createImplicitReleaseBranch(
            CiProcessId processId,
            ConfigBundle bundle,
            OrderedArcRevision revision,
            String login,
            Version version
    ) {
        log.info("Creating implicit release branch for launch ver {} for process {} at {}",
                version, processId, revision);

        Branch branch = createBranchWithAvailableName(bundle, processId, revision, login, version);
        log.info("Branch {} created", branch.getArcBranch());
        return branch;
    }

    private Optional<Branch> findImplicit(ArcRevision revision, CiProcessId processId, Version version) {
        Optional<Branch> implicit = getBranchesAtRevision(revision, processId)
                .stream()
                .filter(p -> p.getItem().getVersion().equals(version))
                .findFirst();

        if (implicit.isPresent()) {
            log.info("Found already created branch {} for version {}", implicit, version);
        }
        return implicit;
    }

    private Branch createBranchWithAvailableName(
            ConfigBundle bundle,
            CiProcessId processId,
            OrderedArcRevision revision,
            String login,
            Version version
    ) {
        ConfigSecurityState securityState = bundle.getConfigEntity().getSecurityState();
        Preconditions.checkArgument(securityState.isValid(), "security state is invalid");

        YavSecret secret = securityAccessService.getYavSecret(securityState.getYavTokenUuid());
        String oauthToken = secret.getCiToken();

        ReleaseConfig releaseConfig = bundle.getValidReleaseConfigOrThrow(processId);
        Preconditions.checkState(releaseConfig.getBranches().isEnabled(),
                "Branches are not configured for release process %s", processId.getSubId()
        );

        var configRevision = bundle.getRevision();
        ArcBranch arcBranch = null;
        for (int seq = 0; seq < CONFLICT_RESOLUTION_MAX_TRIES; seq++) {
            arcBranch = ArcBranch.ofBranchName(
                    nameGenerator.generateName(releaseConfig, version.asString(), seq)
            );

            try {
                return doCreateBranch(configRevision, processId, revision, arcBranch, login, version, oauthToken);
            } catch (BranchConflictException be) {
                log.info("Branch {} already exists", arcBranch);
            }
        }

        throw new RuntimeException(
                "Couldn't create branch for %s tries, last attempt was '%s', try change pattern"
                        .formatted(CONFLICT_RESOLUTION_MAX_TRIES, arcBranch)
        );
    }

    @VisibleForTesting
    public Branch doCreateBranch(
            OrderedArcRevision configRevision,
            CiProcessId processId,
            OrderedArcRevision commit,
            ArcBranch branch,
            String login,
            Version version,
            String arcOAuthToken) {
        log.info("Creating branch {} at revision {} via {}", branch, commit, login);

        var id = BranchInfo.Id.of(branch);
        var existing = db.branches().find(id);
        if (existing.isPresent()) {
            throw new BranchConflictException("Branch %s already exists: %s".formatted(branch, existing.get()));
        }

        try {
            arcService.createReleaseBranch(commit, branch, arcOAuthToken);
            log.info("Branch {} created in arc", branch);
        } catch (StatusRuntimeException e) {
            var prefix = "Unable to create arc branch " + branch;
            log.error("{}", prefix, e);

            var description = e.getStatus().getDescription();
            if (description != null && description.contains("Not fast forward")) {
                throw new BranchConflictException(prefix + ": " + e.getMessage());
            }
            throw new UnknownBranchException(prefix + e.getMessage());
        }

        BranchInfo info = db.branches().save(BranchInfo.builder()
                .id(id)
                .baseRevision(commit)
                .configRevision(configRevision)
                .created(Instant.now(clock))
                .createdBy(login)
                .build()
        );

        CommitRangeService.Range range = commitRangeService.getCommitsToCapture(processId, branch, commit);

        TimelineBranchItem item = db.timelineBranchItems().save(TimelineBranchItem.builder()
                .idOf(processId, branch)
                .state(createState(processId, range))
                .version(version)
                .vcsInfo(createVscInfo(range))
                .build());

        return Branch.of(info, item);
    }

    private BranchState createState(CiProcessId processId, CommitRangeService.Range range) {

        var result = commitRangeService.getCancelledAndDisplacedLaunches(
                processId, range.getRevision(),
                range.getPreviousRevision());

        var cancelledLaunches = result.getCancelled();

        var state = BranchState.builder()
                .cancelledBaseLaunches(cancelledLaunches)
                .build();

        var launches = timelineService.getAtRevision(processId, range.getRevision())
                .stream()
                .map(TimelineItem::getLaunch)
                .filter(Objects::nonNull)
                .collect(Collectors.toList());

        for (Launch launch : launches) {
            state = state.registerLaunch(launch.getLaunchId().getNumber(), launch.getStatus(), false);
        }
        return state;
    }

    private BranchVcsInfo createVscInfo(CommitRangeService.Range range) {
        return BranchVcsInfo.builder()
                .head(range.getRevision())
                .previousRevision(range.getPreviousRevision())
                .trunkCommitCount(range.getCount())
                .branchCommitCount(0)
                .updatedDate(Instant.now(clock))
                .build();
    }

    public Optional<BranchInfo> getBranch(ArcBranch branch) {
        return db.branches().find(BranchInfo.Id.of(branch));
    }

    public Optional<Branch> findBranch(ArcBranch branch, CiProcessId processId) {
        return db.timelineBranchItems().find(TimelineBranchItem.Id.of(processId, branch))
                .map(item -> {
                    BranchInfo info = db.branches().get(item.getId().getInfoId());
                    return Branch.of(info, item);
                });
    }

    public Branch getBranch(ArcBranch branch, CiProcessId processId) {
        var item = db.timelineBranchItems().get(TimelineBranchItem.Id.of(processId, branch));
        var info = db.branches().get(item.getId().getInfoId());
        return Branch.of(info, item);
    }

    public List<Branch> getBranches(CiProcessId processId, @Nullable TimelineBranchItemByUpdateDate.Offset offset,
                                    int limit) {
        List<TimelineBranchItem> items = db.timelineBranchItems().findByUpdateDate(processId, offset, limit);
        return enrich(items);
    }

    public SetMultimap<CommitId, ArcBranch> getBranchesAtRevisions(CiProcessId processId,
                                                                   Collection<? extends CommitId> commits) {
        Map<ArcBranch, CommitId> branches = StreamEx.of(db.branches().findAtCommits(commits))
                .map(BranchInfoByCommitId::getId)
                .toMap(id -> ArcBranch.ofBranchName(id.getBranch()), id -> ArcRevision.of(id.getCommitId()));

        Set<ArcBranch> presentInProcess = StreamEx.of(
                db.timelineBranchItems().findIdsByNames(processId, branches.keySet()))
                .map(id -> ArcBranch.ofBranchName(id.getBranch()))
                .toSet();

        branches.keySet().retainAll(presentInProcess);

        return Multimaps.invertFrom(Multimaps.forMap(branches), HashMultimap.create());
    }

    public Map<CiProcessId, List<Branch>> getTopBranches(List<CiProcessId> processId, int limit) {
        List<TimelineBranchItem> items = db.timelineBranchItems().findLastUpdatedInProcesses(processId, limit);
        List<Branch> branches = enrich(items);
        return branches.stream()
                .collect(Collectors.groupingBy(Branch::getProcessId));
    }

    public List<Branch> getTopBranches(CiProcessId processId, int limit) {
        var top = getTopBranches(List.of(processId), limit);
        return top.getOrDefault(processId, List.of());
    }

    private List<Branch> enrich(Collection<TimelineBranchItem> items) {
        var infoIds = items.stream()
                .map(item -> item.getId().getInfoId())
                .collect(Collectors.toSet());

        var infos = StreamEx.of(db.branches().find(infoIds))
                .toMap(BranchInfo::getId, Function.identity());

        return items.stream()
                .map(item -> {
                    BranchInfo info = infos.get(item.getId().getInfoId());
                    return Branch.of(info, item);
                })
                .collect(Collectors.toList());
    }

    public void addCommit(CiProcessId processId, OrderedArcRevision revision) {
        Optional<Branch> branchOptional = findBranch(revision.getBranch(), processId);
        if (branchOptional.isEmpty()) {
            log.info("Branch not registered in CI, revision {} not added", revision);
            return;
        }
        Branch branch = branchOptional.get();
        TimelineBranchItem branchItem = branch.getItem();
        BranchVcsInfo vcsInfo = branchItem.getVcsInfo();

        int count = Math.max(
                Math.toIntExact(revision.getNumber()),
                vcsInfo.getBranchCommitCount()
        );

        var head = vcsInfo.getHead() == null
                || !vcsInfo.getHead().getBranch().equals(revision.getBranch()) // head в начале смотрит на baseBranch
                || vcsInfo.getHead().isBefore(revision)
                ? revision
                : vcsInfo.getHead(); // голова смотрит дальше, текущий коммит мог быть обработан позже

        TimelineBranchItem updatedItem = branchItem.toBuilder()
                .vcsInfo(vcsInfo.toBuilder()
                        .branchCommitCount(count)
                        .head(head)
                        .updatedDate(Instant.now())
                        .build()
                )
                .build();

        db.timelineBranchItems().save(updatedItem);
        timelineService.addBranch(branch.withItem(updatedItem));
        log.info("Max discovered commit in branch {} updated to {}", updatedItem.getId(), count);
    }

    public void updateTimelineBranchAndLaunchItems(Launch launch) {
        Preconditions.checkNotNull(launch);

        if (launch.getProcessId().getType() != CiProcessId.Type.RELEASE) {
            log.info("Launch {} is not a release, skip updating branch", launch.getId());
            return;
        }

        timelineService.updateTimelineLaunchItem(launch);

        OrderedArcRevision revision = launch.getVcsInfo().getRevision();
        CiProcessId processId = launch.getProcessId();
        ArcBranch selectedBranch = launch.getVcsInfo().getSelectedBranch();
        if (selectedBranch.isPr()) {
            log.info("Pull request meta-branch {}, skipping", selectedBranch);
            return;
        }


        if (!selectedBranch.isTrunk()) {
            // обновляем информацию в ветке, на которой пользователь запустил процесс
            updateTimelineBranchItem(launch, List.of(findByName(processId, selectedBranch)));
        }

        // релиз запущен в транке, обновляем все релизы
        var atRevision = getBranchesAtRevision(revision.toRevision(), processId);

        updateTimelineBranchItem(launch, atRevision);

        log.info("Branch stats updated");
    }

    private Branch findByName(CiProcessId processId, ArcBranch name) {
        var items = db.timelineBranchItems().findByNames(processId, List.of(name));
        Preconditions.checkState(items.size() == 1,
                "Expected single branch item for process %s with name %s, found %s",
                processId, name, items
        );
        var info = db.branches().get(BranchInfo.Id.of(name));
        return Branch.of(info, items.get(0));
    }

    private void updateTimelineBranchItem(Launch launch, List<Branch> branches) {
        StreamEx.of(branches)
                .map(branch -> {
                    log.info("Updating branch {} according to related launch: {}",
                            branch.getInfo().getArcBranch(), launch.getId());

                    int launchNumber = launch.getLaunchId().getNumber();
                    BranchState state = branch.getState().registerLaunch(launchNumber, launch.getStatus(), false);
                    state = countFreeCommitsInBranch(launch, branch.getArcBranch(), state);
                    var updatedItem = branch.getItem().toBuilder()
                            .state(state)
                            .build();
                    return Branch.of(branch.getInfo(), updatedItem);
                })
                .forEach(updated -> {
                    db.timelineBranchItems().save(updated.getItem());
                    timelineService.branchUpdated(updated);
                });
    }

    private BranchState countFreeCommitsInBranch(Launch updatedLaunch, ArcBranch branch, BranchState branchState) {
        long prevRevisionNumber = -1;
        if (branchState.getLastLaunchNumber() > 0) {
            prevRevisionNumber = branchState.getLastLaunchNumber() == updatedLaunch.getLaunchId().getNumber()
                    ? updatedLaunch.getVcsInfo().getRevision().getNumber()
                    : db.launches()
                    .get(LaunchId.of(updatedLaunch.getProcessId(), branchState.getLastLaunchNumber()))
                    .getVcsInfo().getRevision().getNumber();
        }

        return branchState.toBuilder()
                .freeCommits(db.discoveredCommit().count(updatedLaunch.getProcessId(), branch, -1, prevRevisionNumber))
                .build();
    }

    private List<Branch> getBranchesAtRevision(ArcRevision revision, CiProcessId processId) {
        List<BranchInfo> atCommit = db.branches().findAtCommit(revision);
        log.info("Found {} branches at {}: {}", atCommit.size(), revision, atCommit);

        Map<ArcBranch, BranchInfo> byName = StreamEx.of(atCommit)
                .toMap(BranchInfo::getArcBranch, Function.identity());

        List<TimelineBranchItem> branchItems = db.timelineBranchItems().findByNames(processId, byName.keySet());
        log.info("Found {} branches for process {}: {}", branchItems.size(), processId,
                branchItems.stream()
                        .map(TimelineBranchItem::getArcBranch)
                        .collect(Collectors.toList())
        );

        return StreamEx.of(branchItems)
                .map(item -> {
                    BranchInfo info = byName.get(item.getArcBranch());
                    Preconditions.checkState(info != null, "Not found branch info %s", item.getArcBranch());
                    return Branch.of(info, item);
                })
                .toList();

    }

    public Set<CiProcessId> getProcessesForBranch(ArcBranch branch) {
        return db.timelineBranchItems().findByName(branch)
                .stream()
                .map(TimelineBranchItem::getProcessId)
                .collect(Collectors.toSet());
    }

    public void addProcessedCommitToBranchItemStats(Set<TimelineBranchItem.Id> ids) {
        var updatedBranches = db.timelineBranchItems().find(ids).stream()
                .map(item -> {
                    var branchState = item.getState().incrementNumberOfCommits(1);
                    return item.toBuilder()
                            .state(branchState)
                            .build();
                }).collect(Collectors.toList());

        db.timelineBranchItems().save(updatedBranches);
    }
}
