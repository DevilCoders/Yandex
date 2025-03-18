package ru.yandex.ci.core.launch;

import java.nio.file.Path;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.Getter;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class LaunchTable extends KikimrTableCi<Launch> {

    public LaunchTable(QueryExecutor executor) {
        super(Launch.class, executor);
    }

    /**
     * Find launches by version
     *
     * @param processId process id
     * @param version   version to find
     * @return launch or raise an exception if more than 1 launch of same version was found
     */
    public Optional<Launch> findLaunchesByVersion(CiProcessId processId, Version version) {
        var res = find(YqlPredicate.where("id.processId").eq(processId.asString())
                        .and("versionString").eq(version.asString()),
                YqlView.index(Launch.IDX_PROCESS_ID_VERSION),
                YqlLimit.top(2));
        if (res.isEmpty()) {
            return Optional.empty();
        } else if (res.size() == 1) {
            return Optional.of(res.get(0));
        } else {
            throw new RuntimeException("Found more than 2 launches of [%s] with same version [%s]"
                    .formatted(processId, version));
        }
    }

    public LaunchAccessView getLaunchAccessView(LaunchId id) {
        return get(LaunchAccessView.class, id.toKey());
    }

    public LaunchStatusView getLaunchStatusView(Launch.Id id) {
        return get(LaunchStatusView.class, id);
    }

    public boolean hasLaunchesByVersion(CiProcessId processId, Version version) {
        return count(YqlPredicate.where("id.processId").eq(processId.asString())
                        .and("versionString").eq(version.asString()),
                YqlView.index(Launch.IDX_PROCESS_ID_VERSION)) > 0;
    }


    public List<Launch.Id> getDelayedLaunchIds(Path configPath, CommitId configRevision) {
        return findIds(YqlPredicate
                        .where("configPath").eq(configPath.toString())
                        .and("configCommitId").eq(configRevision.getCommitId())
                        .and("status").eq(LaunchState.Status.DELAYED),
                YqlView.index(Launch.IDX_PATH_COMMIT_STATUS));
    }

    public List<Launch> getLaunches(
            CiProcessId processId,
            LaunchTableFilter filter,
            int offsetLaunchNumber,
            int limit
    ) {
        return executor.execute(
                new LaunchTableFilterStatement(processId, filter, offsetLaunchNumber, limit),
                filter
        );
    }

    public List<String> getTagsStartsWith(CiProcessId processId, String tag, int offset, int limit) {
        List<YqlStatementPart<?>> parts = new ArrayList<>();
        parts.add(YqlPredicate.eq(LaunchByProcessIdAndTag.PROCESS_ID_FIELD, processId.asString())
                .and(YqlPredicate.like(LaunchByProcessIdAndTag.TAG_FIELD, tag + '%'))
        );
        parts.add(YqlOrderBy.orderBy(LaunchByProcessIdAndTag.TAG_FIELD, YqlOrderBy.SortOrder.ASC));
        addOffsetAndLimit(parts, offset, limit);

        return launchByProcessIdAndTag().findDistinct(TagView.class, parts)
                .stream()
                .map(TagView::getTag)
                .collect(Collectors.toList());
    }

    public Optional<String> getBranchByName(CiProcessId processId, String branch) {
        List<YqlStatementPart<?>> parts = new ArrayList<>();
        parts.add(YqlPredicate.eq(LaunchByProcessIdAndArcBranch.PROCESS_ID_FIELD, processId.asString())
                .and(YqlPredicate.eq(LaunchByProcessIdAndArcBranch.BRANCH_FIELD, branch))
        );
        addOffsetAndLimit(parts, 0, 1);

        return launchByProcessIdAndArcBranch().find(parts)
                .stream()
                .map(LaunchByProcessIdAndArcBranch::getId)
                .map(LaunchByProcessIdAndArcBranch.Id::getBranch)
                .findFirst();
    }

    public List<String> getBranchesStartsWith(CiProcessId processId, String prefix, int offset, int limit) {
        List<YqlStatementPart<?>> parts = new ArrayList<>();
        parts.add(YqlPredicate.eq(LaunchByProcessIdAndArcBranch.PROCESS_ID_FIELD, processId.asString())
                .and(YqlPredicate.like(LaunchByProcessIdAndArcBranch.BRANCH_FIELD, prefix + '%'))
        );
        parts.add(YqlOrderBy.orderBy(LaunchByProcessIdAndArcBranch.BRANCH_FIELD, YqlOrderBy.SortOrder.ASC));
        addOffsetAndLimit(parts, offset, limit);

        return launchByProcessIdAndArcBranch().findDistinct(BranchView.class, parts)
                .stream()
                .map(BranchView::getBranch)
                .collect(Collectors.toList());
    }

    public List<String> getBranchesBySubstring(CiProcessId processId, String substring, int offset, int limit) {
        List<YqlStatementPart<?>> parts = new ArrayList<>();
        parts.add(YqlPredicate.eq(LaunchByProcessIdAndArcBranch.PROCESS_ID_FIELD, processId.asString())
                .and(YqlPredicate.like(LaunchByProcessIdAndArcBranch.BRANCH_FIELD, '%' + substring + '%'))
        );
        parts.add(YqlOrderBy.orderBy(LaunchByProcessIdAndArcBranch.BRANCH_FIELD, YqlOrderBy.SortOrder.ASC));
        addOffsetAndLimit(parts, offset, limit);

        return launchByProcessIdAndArcBranch().findDistinct(BranchView.class, parts)
                .stream()
                .map(BranchView::getBranch)
                .collect(Collectors.toList());
    }

    public List<Launch.Id> getPullRequestLaunches(long pullRequestId, long diffSetId) {
        var statements = List.of(
                YqlPredicate.where("id.pullRequestId").eq(pullRequestId),
                YqlPredicate.where("id.diffSetId").eq(diffSetId),
                YqlOrderBy.orderBy("id"));
        var list = launchByPullRequest().find(statements);
        return list.stream()
                .map(LaunchByPullRequest::getId)
                .map(LaunchByPullRequest.Id::getId)
                .collect(Collectors.toList());
    }

    public List<CountByProcessIdAndStatus> getActiveReleaseLaunchesCount(@Nonnull String project) {
        return getActiveReleaseLaunchesCount(project, null);
    }

    public List<CountByProcessIdAndStatus> getActiveReleaseLaunchesCount(
            @Nonnull String project,
            @Nullable CiProcessId processId
    ) {
        var wherePredicate = YqlPredicate.where("project").eq(project)
                .and(YqlPredicate.where("processType").eq(CiProcessId.Type.RELEASE))
                .and(YqlPredicateCi.in("status", LaunchState.Status.nonTerminalStatuses()));

        if (processId != null) {
            wherePredicate = wherePredicate.and(YqlPredicate.where("id.processId").eq(processId.asString()));
        }

        return this.groupBy(
                CountByProcessIdAndStatus.class,
                List.of("processId", "status", "branch"),
                List.of(
                        "processId",
                        "status",
                        "COALESCE(" +
                                "selectedBranch," +
                                "cast('" + ArcBranch.trunk().asString() + "' as Utf8) " +
                                ") AS branch"
                ),
                List.of(
                        wherePredicate,
                        YqlView.index(Launch.IDX_PROJECT_PROCESS_TYPE_STATUS)
                ))
                .stream()
                .filter(it -> it.getCount() > 0)
                .collect(Collectors.toList());
    }

    @Override
    public void deleteAll() {
        super.deleteAll();
        launchByProcessIdAndArcBranch().deleteAll();
        launchByProcessIdAndPinned().deleteAll();
        launchByProcessIdAndStatus().deleteAll();
        launchByProcessIdAndTag().deleteAll();
        launchByPullRequest().deleteAll();
    }

    private KikimrTable<LaunchByProcessIdAndArcBranch> launchByProcessIdAndArcBranch() {
        return new KikimrTable<>(LaunchByProcessIdAndArcBranch.class, executor);
    }

    private KikimrTable<LaunchByProcessIdAndPinned> launchByProcessIdAndPinned() {
        return new KikimrTable<>(LaunchByProcessIdAndPinned.class, executor);
    }

    private KikimrTable<LaunchByProcessIdAndStatus> launchByProcessIdAndStatus() {
        return new KikimrTable<>(LaunchByProcessIdAndStatus.class, executor);
    }

    private KikimrTable<LaunchByProcessIdAndTag> launchByProcessIdAndTag() {
        return new KikimrTable<>(LaunchByProcessIdAndTag.class, executor);
    }

    private KikimrTableCi<LaunchByPullRequest> launchByPullRequest() {
        return new KikimrTableCi<>(LaunchByPullRequest.class, executor);
    }

    private static void addOffsetAndLimit(List<YqlStatementPart<?>> parts, int offset, int limit) {
        parts.addAll(filter(limit, offset));
    }

    public Launch get(LaunchId launchId) {
        return get(launchId.toKey());
    }

    public Optional<Launch> findOptional(LaunchId launchId) {
        return find(launchId.toKey());
    }

    public List<Launch> getProcessLaunches(Collection<LaunchId> launchIds) {
        if (launchIds.isEmpty()) {
            return Collections.emptyList();
        }
        CiProcessId processId = launchIds.iterator().next().getProcessId();
        Preconditions.checkArgument(
                launchIds.stream().allMatch(id -> id.getProcessId().equals(processId)),
                "all launches should be from same process, got %s",
                launchIds
        );
        Set<Integer> numbers = launchIds.stream()
                .map(LaunchId::getNumber)
                .collect(Collectors.toSet());

        return find(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString())
                        .and("id.launchNumber").in(numbers)
        ));
    }

    public Optional<Launch> getLastNotCancelledLaunch(CiProcessId processId) {
        List<Launch> launches = find(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString())
                        .and("status").neq(LaunchState.Status.CANCELED),
                YqlOrderBy.orderBy("id.launchNumber", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1)
        ));
        return launches.stream().findFirst();
    }

    public Optional<Launch> getLastNotCancelledLaunch(CiProcessId processId, int beforeLaunchNumber) {
        List<Launch> launches = find(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicate.where("id.launchNumber").lt(beforeLaunchNumber),
                YqlPredicate.where("status").neq(LaunchState.Status.CANCELED),
                YqlOrderBy.orderBy("id.launchNumber", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1)
        ));
        return launches.stream().findFirst();
    }

    public Optional<Launch> getLastFinishedLaunch(CiProcessId processId) {
        List<Launch> launches = find(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicate.where("status").eq(LaunchState.Status.SUCCESS),
                YqlView.index(Launch.IDX_PROCESS_ID_STATUS),
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1)
        ));
        return launches.stream().findFirst();
    }

    public List<Launch> getActiveLaunches(CiProcessId processId) {
        return find(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicateCi.notIn("status", LaunchState.Status.terminalStatuses()),
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.DESC)
        ));
    }

    public List<Launch.Id> getActiveLaunchIds(CiProcessId processId) {
        return findIds(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicateCi.notIn("status", LaunchState.Status.terminalStatuses()),
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.DESC)
        ));
    }

    public List<Launch.Id> getLaunchIds(
            CiProcessId processId,
            ArcBranch branch,
            LaunchState.Status status,
            long limit
    ) {
        return findIds(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicate.where("status").eq(status),
                YqlPredicate.where("selectedBranch").eq(branch.getBranch()),
                YqlView.index(Launch.IDX_PROCESS_ID_STATUS),
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.ASC),
                YqlLimit.top(limit)
        ));
    }

    public long getLaunchesCountExcept(
            CiProcessId processId,
            ArcBranch branch,
            Set<LaunchState.Status> excludeStatuses
    ) {
        return count(List.of(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicateCi.notIn("status", excludeStatuses),
                YqlPredicate.where("selectedBranch").eq(branch.getBranch())
        ));
    }

    public List<LaunchVersionView> getAllActiveLaunchVersions() {
        return find(LaunchVersionView.class,
                YqlPredicate.where("processType").eq(CiProcessId.Type.RELEASE),
                YqlPredicateCi.notIn("status", LaunchState.Status.terminalStatuses()),
                YqlOrderBy.orderBy("id")
        );
    }

    public List<LaunchFinishedView> getAllLatestLaunches() {
        return this.groupBy(
                LaunchFinishedView.class,
                List.of("processId", "status", "flowId", "max(finished) as finished"),
                List.of("processId", "status", "cast(json_value(flowInfo, '$.flowId.id') as Utf8) as flowId"),
                List.of(
                        YqlPredicate.where("processType").eq(CiProcessId.Type.RELEASE),
                        YqlPredicate.where("status").eq(LaunchState.Status.SUCCESS),
                        YqlPredicate.where("finished").isNotNull()
                ));
    }

    public List<Launch> getLaunches(CiProcessId processId, int offsetLaunchNumber, int limit) {
        return getLaunches(processId, offsetLaunchNumber, limit, true);
    }

    public List<Launch> getLaunches(
            CiProcessId processId,
            int offsetLaunchNumber,
            int limit,
            boolean excludeCanceled
    ) {
        var parts = filter(limit);
        parts.add(YqlPredicate.where("id.processId").eq(processId.asString()));
        if (offsetLaunchNumber > 0) {
            parts.add(YqlPredicate.where("id.launchNumber").lt(offsetLaunchNumber));
        }
        if (excludeCanceled) {
            parts.add(YqlPredicate.where("status").neq(LaunchState.Status.CANCELED));
        }
        parts.add(YqlOrderBy.orderBy("id.launchNumber", YqlOrderBy.SortOrder.DESC));
        return find(parts);
    }

    public List<Launch.Id> getActiveLaunchIdsWithoutPostpone(CiProcessId processId) {
        var statuses = new HashSet<>(LaunchState.Status.terminalStatuses());
        statuses.add(LaunchState.Status.POSTPONE);
        return findIds(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicateCi.notIn("status", statuses),
                YqlPredicate.where("selectedBranch").eq(ArcBranch.trunk().getBranch()),
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.ASC)
        );
    }

    public List<Launch.Id> getLaunchIds(CiProcessId processId, ArcBranch branch, LaunchState.Status status) {
        return findIds(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicate.where("status").eq(status),
                YqlPredicate.where("selectedBranch").eq(branch.getBranch()),
                YqlView.index(Launch.IDX_PROCESS_ID_STATUS),
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.ASC)
        );
    }

    public void findStats(Instant since, Consumer<ProcessStat> statConsumer) {
        var ids = new HashSet<Launch.Id>(findIds(
                YqlPredicate.where("activityChanged").gt(since),
                YqlView.index(Launch.IDX_ACTIVITY_CHANGED)
        ));

        find(ProcessStatView.class, ids).stream()
                .collect(Collectors.groupingBy(
                        it -> ProcessIdActivity.of(it.getProcessId(), it.getActivity()),
                        Collectors.mapping(
                                it -> new ProcessStat(
                                        it.getProcessId(), it.getProject(), it.getActivity(),
                                        1,
                                        Optional.ofNullable(it.getStatistics())
                                                .map(LaunchStatistics::getRetries)
                                                .map(retries -> retries > 0 ? 1 : 0)
                                                .orElse(0)
                                ),
                                Collectors.reducing(ProcessStat::merge)
                        )
                ))
                .values()
                .forEach(it -> it.ifPresent(statConsumer));
    }

    @Value
    public static class TagView implements View {
        @Column(name = "idx_tag", dbType = DbType.UTF8)
        String tag;
    }

    @Value
    public static class BranchView implements View {
        @Column(name = "idx_branch", dbType = DbType.UTF8)
        String branch;
    }

    @Value
    public static class ProcessStatView implements View {
        @Column(dbType = DbType.UTF8)
        String processId;

        @Column
        int launchNumber;

        @Column(dbType = DbType.UTF8)
        String project;

        @Column(dbType = DbType.UTF8)
        Activity activity;

        @Nullable
        @Column(dbType = DbType.JSON, flatten = false)
        LaunchStatistics statistics;
    }

    @Value(staticConstructor = "of")
    public static class ProcessIdActivity {
        String processId;
        Activity activity;
    }

    @Value
    public static class LaunchAccessView implements Table.View {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "launchNumber")
        int launchNumber;

        @Column(dbType = DbType.JSON, flatten = false)
        LaunchFlowInfo flowInfo;

        public CiProcessId getCiProcessId() {
            return CiProcessId.ofString(processId);
        }
    }

    @Value
    public static class LaunchStatusView implements Table.View {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "launchNumber")
        int launchNumber;

        @Column(dbType = DbType.UTF8)
        LaunchState.Status status;

        public LaunchId getLaunchId() {
            return LaunchId.of(CiProcessId.ofString(processId), launchNumber);
        }
    }

    @Value
    public static class LaunchVersionView implements Table.View {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "launchNumber")
        int launchNumber;

        @Column(dbType = DbType.UTF8)
        LaunchState.Status status;

        @Nullable
        @Getter(AccessLevel.PRIVATE)
        @Column(dbType = DbType.UTF8, name = "version")
        String versionString;

        @Nullable
        @Column(dbType = DbType.JSON, flatten = false, name = "ver")
        Version version;

        public LaunchId getLaunchId() {
            return LaunchId.of(CiProcessId.ofString(processId), launchNumber);
        }

        public Version getVersion() {
            if (version == null) {
                Preconditions.checkState(versionString != null,
                        "Internal error, both version and versionString are null in launch %s", getLaunchId());
                return Version.fromAsString(versionString);
            }
            return version;
        }
    }

    @Value
    public static class LaunchFinishedView implements Table.View {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "status", dbType = DbType.UTF8)
        LaunchState.Status status;

        @Column(name = "flowId", dbType = DbType.UTF8)
        String flowId;

        @Column(name = "finished", dbType = DbType.TIMESTAMP)
        Instant finished;

        @Column(name = "count", dbType = DbType.UINT64)
        long count;
    }

    @Value
    public static class CountByProcessIdAndStatus implements View {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "status", dbType = DbType.UTF8)
        LaunchState.Status status;

        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;

        @Column(name = "count", dbType = DbType.UINT64)
        long count; // Count should has UINT64 type. When UINT32, database returns zero instead of actual value

        public CiProcessId getProcessId() {
            return CiProcessId.ofString(processId);
        }
    }
}
