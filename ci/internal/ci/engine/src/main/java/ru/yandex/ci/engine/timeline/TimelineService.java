package ru.yandex.ci.engine.timeline;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.core.timeline.TimelineItemEntity;
import ru.yandex.ci.engine.branch.BranchTraverseService;
import ru.yandex.ci.engine.launch.CanStartLaunchAtRevision;

@RequiredArgsConstructor
public class TimelineService {
    private static final Logger log = LoggerFactory.getLogger(TimelineService.class);

    public static final String TIMELINE_VERSION_NAMESPACE = "timeline-version";
    private static final String TIMELINE_NAMESPACE = "timeline";

    private final CiMainDb db;
    private final RevisionNumberService revisionNumberService;
    private final BranchTraverseService branchTraverseService;

    public void updateTimelineLaunchItem(Launch launch) {
        CiProcessId processId = launch.getLaunchId().getProcessId();
        if (processId.getType() != CiProcessId.Type.RELEASE) {
            log.info("Launch {} is not a release, skip adding to timeline", launch.getId());
            return;
        }

        OrderedArcRevision revision = launch.getVcsInfo().getRevision();
        Preconditions.checkNotNull(revision);

        long timelineVersion = nextTimelineVersion(processId);
        List<TimelineItemEntity> revisionItems = db.timeline().getAtRevision(processId, revision);

        TimelineItemEntity item = findLaunch(revisionItems, launch.getId())
                .orElseGet(() -> createEntity(launch.getId(), null, processId, revision));

        TimelineItemEntity updated = item.toBuilder()
                .launch(launch.getId())
                .hidden(isHidden(launch))
                .showInBranch(showInBranch(revisionItems, launch))
                .inStable(launch.getStatus() == LaunchState.Status.SUCCESS)
                .timelineVersion(timelineVersion)
                .status(launch.getStatus())
                .started(launch.getStarted())
                .finished(launch.getFinished())
                .build();

        db.timeline().save(updated);
    }

    public void branchUpdated(Branch updatedBranch) {
        var inStable = !updatedBranch.getItem().getState().getCompletedLaunches().isEmpty();
        if (!inStable) {
            log.info("timeline item for branch {} not in stable, nothing to update", updatedBranch.getArcBranch());
            return;
        }
        List<TimelineItemEntity> revisionItems =
                db.timeline().getAtRevision(updatedBranch.getProcessId(), updatedBranch.getInfo().getBaseRevision());

        var timelineItem = findBranch(revisionItems, updatedBranch.getId())
                .orElseThrow(() -> new IllegalStateException(
                        "not found TimelineBranchItem %s among items %s: processId %s, baseRevision %s".formatted(
                                updatedBranch.getId(), revisionItems,
                                updatedBranch.getProcessId(), updatedBranch.getInfo().getBaseRevision()
                        )
                ));
        if (timelineItem.getInStable()) {
            log.info("timeline item for branch {} has not been changed, skip updating", updatedBranch.getArcBranch());
            return;
        }
        var update = timelineItem.toBuilder()
                .inStable(true)
                .build();

        db.timeline().save(update);
    }

    private String showInBranch(List<TimelineItemEntity> revisionItems, Launch launch) {
        var byItemNumber = Comparator.<TimelineItemEntity, Integer>comparing(item -> item.getId().getItemNumber());

        return revisionItems
                .stream()
                .filter(item -> item.getBranch() != null)
                .sorted(byItemNumber.reversed())
                .map(item -> item.getBranch().getBranch())
                .findFirst()
                .orElse(launch.getVcsInfo().getSelectedBranch().asString());
    }

    private boolean isHidden(Launch launch) {
        return launch.getStatus() == LaunchState.Status.CANCELED;
    }

    private long nextTimelineVersion(CiProcessId processId) {
        var timelineVersion = db.counter().incrementAndGet(TIMELINE_VERSION_NAMESPACE, processId.asString());
        log.info("Incremented timeline version: {}", timelineVersion);
        return timelineVersion;
    }

    public long currentTimelineVersion(CiProcessId processId) {
        return db.counter().get(TIMELINE_VERSION_NAMESPACE, processId.asString()).orElse(-1L);
    }

    public void addBranch(Branch branch) {
        OrderedArcRevision revision = revisionNumberService.getOrderedArcRevision(
                ArcBranch.trunk(),
                branch.getInfo().getBaseRevision()
        );

        CiProcessId processId = branch.getProcessId();
        long timelineVersion = nextTimelineVersion(processId);
        List<TimelineItemEntity> revisionItems = new ArrayList<>(db.timeline().getAtRevision(processId, revision));

        var id = branch.getId();

        Optional<TimelineItemEntity> existing = findBranch(revisionItems, id);
        if (existing.isPresent()) {
            log.info("Branch {} already added", existing.get());
            return;
        }

        var entity = createEntity(null, id, processId, revision);
        log.info("Creating new timeline item {}", entity);
        revisionItems.add(entity);
        revisionItems.stream()
                .map(item -> {
                    TimelineItemEntity.Builder builder = item.toBuilder();
                    if (item.getBranch() != null) {
                        builder.showInBranch(branch.getInfo().getBaseRevision().getBranch().asString());
                    } else {
                        builder.showInBranch(branch.getInfo().getArcBranch().asString());
                    }
                    builder.timelineVersion(timelineVersion);
                    return builder.build();
                })
                .forEach(db.timeline()::save);
    }

    private static Optional<TimelineItemEntity> findLaunch(List<TimelineItemEntity> items, Launch.Id launchId) {
        return items.stream()
                .filter(i -> launchId.equals(i.getLaunch()))
                .findFirst();
    }

    private static Optional<TimelineItemEntity> findBranch(List<TimelineItemEntity> items,
                                                           TimelineBranchItem.Id branchId) {
        return items.stream()
                .filter(i -> branchId.equals(i.getBranch()))
                .findFirst();
    }

    private TimelineItemEntity createEntity(
            @Nullable Launch.Id launchId,
            @Nullable TimelineBranchItem.Id branchId,
            CiProcessId processId, OrderedArcRevision revision) {
        int itemNumber = nextItemNumber(processId, revision);

        TimelineItemEntity.Id id = TimelineItemEntity.Id.of(processId, revision, itemNumber);
        return TimelineItemEntity.builder()
                .id(id)
                .launch(launchId)
                .branch(branchId)
                .build();
    }

    private int nextItemNumber(CiProcessId processId, OrderedArcRevision revision) {
        return (int) db.counter().incrementAndGet(
                TIMELINE_NAMESPACE, revision.getBranch().asString() + ":" + processId.asString()
        );
    }

    public Optional<TimelineItem> getPreviousStable(Launch launch) {
        var revision = launch.getVcsInfo().getRevision();
        var revisionItems = db.timeline().getAtRevision(launch.getProcessId(), revision);
        var timelineItemId = findLaunch(revisionItems, launch.getId())
                .map(TimelineItemEntity::getId)
                .orElseThrow(
                        () -> new RuntimeException("not found timeline item for launch %s".formatted(launch.getId()))
                );
        Optional<TimelineItemEntity> before = db.timeline().getStableBefore(timelineItemId);

        if (before.isEmpty()) {
            var baseRevision = branchTraverseService.findBaseRevision(revision.getBranch());
            if (baseRevision.isPresent()) {
                before = db.timeline().getStableBefore(
                        TimelineItemEntity.Id.of(launch.getProcessId(), baseRevision.get(), 0)
                );
            }
        }
        return before.map(this::enrich);
    }

    public Optional<TimelineItem> getNextAfter(Launch launch) {
        var revisionItems = db.timeline().getAtRevision(launch.getProcessId(), launch.getVcsInfo().getRevision());

        Optional<TimelineItemEntity> sameRevisionBranch = revisionItems.stream()
                .filter(item -> item.getBranch() != null)
                .findFirst();
        if (sameRevisionBranch.isPresent()) {
            log.info("Found branch at same revision: {}", sameRevisionBranch.get().getBranch());
            return sameRevisionBranch.map(this::enrich);
        }

        var timelineItemId = findLaunch(revisionItems, launch.getId())
                .map(TimelineItemEntity::getId)
                .orElseThrow(
                        () -> new RuntimeException("not found timeline item for launch %s".formatted(launch.getId()))
                );

        Optional<TimelineItemEntity> after = db.timeline().getAfter(
                timelineItemId,
                launch.getVcsInfo().getRevision().getBranch().isTrunk()
        );
        after.ifPresentOrElse(
                afterItem -> log.info("Item after {} is {}", timelineItemId, afterItem.getId()),
                () -> log.info("Not found items after {}", timelineItemId)
        );
        return after.map(this::enrich);
    }

    public List<TimelineItem> getAtRevision(CiProcessId processId, OrderedArcRevision revision) {
        return enrich(db.timeline().getAtRevision(processId, revision));
    }

    public List<TimelineItem> getTimeline(CiProcessId processId, ArcBranch branch, Offset offset, int limit) {
        return db.currentOrReadOnly(() -> {
            List<TimelineItemEntity> items = db.timeline().getTimeline(processId, branch, offset, limit);
            return enrich(items);
        });
    }

    public Optional<TimelineItem> getLastStarted(CiProcessId ciProcessId, ArcBranch branch) {
        return db.currentOrReadOnly(() ->
                getLastStartedImpl(ciProcessId, branch)
                        .map(item -> enrich(item).withShowInBranch(item.getShowInBranch())));
    }

    private Optional<TimelineItemEntity> getLastStartedImpl(CiProcessId ciProcessId, ArcBranch branch) {
        var lastStarted = db.timeline().getLastStarted(ciProcessId);
        if (lastStarted.isEmpty()) {
            return Optional.empty();
        }

        var lastBranch = Optional.ofNullable(lastStarted.get().getShowInBranch())
                .orElse(ArcBranch.trunk().asString());
        if (!lastBranch.equals(branch.asString())) {
            return lastStarted;
        }

        var firstInBranch = db.timeline().getTimeline(ciProcessId, branch, Offset.EMPTY, 1).stream().findFirst();
        if (firstInBranch.isEmpty()) {
            return lastStarted;
        }

        var first = firstInBranch.get();
        return lastStarted
                .filter(last -> !first.getId().equals(last.getId()));
    }

    private TimelineItem enrich(TimelineItemEntity item) {
        return enrich(List.of(item)).get(0);
    }

    private List<TimelineItem> enrich(List<TimelineItemEntity> items) {
        var branchIds = items.stream()
                .map(TimelineItemEntity::getBranch)
                .filter(Objects::nonNull)
                .collect(Collectors.toSet());

        var branchInfoIds = branchIds.stream()
                .map(id -> BranchInfo.Id.of(id.getBranch()))
                .collect(Collectors.toSet());

        Map<BranchInfo.Id, BranchInfo> branchInfos = db.branches().find(branchInfoIds)
                .stream()
                .collect(Collectors.toMap(BranchInfo::getId, Function.identity()));

        Map<TimelineBranchItem.Id, TimelineBranchItem> branchItems = db.timelineBranchItems().find(branchIds)
                .stream()
                .collect(Collectors.toMap(TimelineBranchItem::getId, Function.identity()));

        var launchIds = Stream.concat(
                items.stream()
                        .map(TimelineItemEntity::getLaunch)
                        .filter(Objects::nonNull),
                branchItems.values().stream()
                        .filter(item -> item.getState().getLastLaunchNumber() > -1)
                        .map(item -> Launch.Id.of(
                                item.getProcessId().asString(),
                                item.getState().getLastLaunchNumber()
                        ))
        ).collect(Collectors.toSet());

        Map<Launch.Id, Launch> launches = db.launches().find(launchIds)
                .stream()
                .collect(Collectors.toMap(Launch::getId, Function.identity()));

        return items.stream()
                .map(item -> createItem(item, launches, branchInfos, branchItems))
                .collect(Collectors.toList());
    }

    private static TimelineItem createItem(TimelineItemEntity entity,
                                           Map<Launch.Id, Launch> launches,
                                           Map<BranchInfo.Id, BranchInfo> branchInfos,
                                           Map<TimelineBranchItem.Id, TimelineBranchItem> branchItems) {
        TimelineItem.Builder builder = TimelineItem.builder()
                .processId(entity.getProcessId())
                .revision(entity.getId().getRevision())
                .number(entity.getId().getItemNumber())
                .inStable(entity.getInStable());

        if (entity.getLaunch() != null) {
            Launch launch = launches.get(entity.getLaunch());
            Preconditions.checkState(launch != null,
                    "Launch %s from item %s not found",
                    entity.getLaunch(), entity.getId()
            );
            builder.launch(launch);
        }
        if (entity.getBranch() != null) {
            TimelineBranchItem branchItem = branchItems.get(entity.getBranch());
            Preconditions.checkState(branchItem != null,
                    "Branch %s from item %s not found",
                    entity.getBranch(), entity.getId()
            );

            BranchInfo branchInfo = branchInfos.get(entity.getBranch().getInfoId());
            Preconditions.checkState(branchInfo != null,
                    "Branch info %s from item %s not found",
                    entity.getBranch(), entity.getId()
            );

            builder.branch(Branch.of(branchInfo, branchItem));
        }
        return builder.build();
    }

    /**
     * Возвращает хронологически первый элемент, более старый.
     */
    public Optional<TimelineItem> getFirstIn(CiProcessId processId, ArcBranch branch) {
        return db.timeline().getFirstIn(processId, branch).map(this::enrich);
    }

    /**
     * Возвращает хронологически последний элемент, самый верхний, самый молодой.
     */
    public Optional<TimelineItem> getLastIn(CiProcessId processId, ArcBranch branch) {
        return getTimeline(processId, branch, Offset.EMPTY, 1).stream().findFirst();
    }

    public CanStartLaunchAtRevision canStartReleaseWithoutCommits(
            TimelineItem topCapturedTimelineItem,
            ArcBranch selectedBranch,
            boolean allowLaunchEarlier
    ) {
        if (topCapturedTimelineItem.getType() != TimelineItem.Type.LAUNCH && !allowLaunchEarlier) {
            return CanStartLaunchAtRevision.forbidden(
                    "forbidden at branch %s, cause commit belongs to branch %s".formatted(
                            selectedBranch, topCapturedTimelineItem.getBranch().getArcBranch()));
        }
        if (topCapturedTimelineItem.getType() == TimelineItem.Type.LAUNCH &&
                !topCapturedTimelineItem.getLaunch().getStatus().isTerminal()) {
            return CanStartLaunchAtRevision.forbidden(
                    "the latest launch %s is not in terminal state, but in state %s"
                            .formatted(
                                    topCapturedTimelineItem.getLaunch().getVersion().asString(),
                                    topCapturedTimelineItem.getLaunch().getState()
                            ));
        }
        return CanStartLaunchAtRevision.allowed();
    }
}
