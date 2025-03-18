package ru.yandex.ci.core.launch;

import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy.SortOrder;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;

@Slf4j
public class PostponeLaunchTable extends KikimrTableCi<PostponeLaunch> {

    public PostponeLaunchTable(KikimrTable.QueryExecutor executor) {
        super(PostponeLaunch.class, executor);
    }

    public List<ProcessIdView> findIncompleteProcesses(ArcBranch branch, List<VirtualType> virtualTypes) {
        return findProcesses(branch, virtualTypes, List.of(PostponeStatus.NEW, PostponeStatus.STARTED));
    }

    public List<ProcessIdView> findProcesses(
            ArcBranch branch,
            List<VirtualType> virtualTypes,
            List<PostponeStatus> statuses
    ) {
        return findDistinct(ProcessIdView.class, List.of(
                YqlPredicate.where("id.branch").eq(branch.getBranch()),
                YqlPredicate.where("status").in(statuses),
                virtualTypes.isEmpty()
                        ? YqlPredicate.where("virtualType").isNull()
                        : YqlPredicateCi.in("virtualType", virtualTypes),
                YqlOrderBy.orderBy("id.processId")
        ));
    }

    public List<PostponeLaunch> findAll(
            @Nullable VirtualType virtualType,
            ArcBranch branch,
            PostponeStatus... statuses
    ) {
        return find(
                YqlPredicate.where("virtualType").eq(virtualType),
                YqlPredicate.where("id.branch").eq(branch.getBranch()),
                YqlPredicateCi.in("status", statuses)
        );
    }

    public Optional<PostponeLaunch> firstIncompleteLaunch(
            ProcessIdView processIdView,
            ArcBranch branch,
            List<VirtualType> virtualTypes
    ) {
        return find(
                YqlPredicate.where("id.processId").eq(processIdView.getProcessId()),
                YqlPredicate.where("id.branch").eq(branch.getBranch()),
                YqlPredicateCi.in("status", PostponeStatus.NEW, PostponeStatus.STARTED),
                virtualTypes.isEmpty()
                        ? YqlPredicate.where("virtualType").isNull()
                        : YqlPredicateCi.in("virtualType", virtualTypes),
                YqlOrderBy.orderBy("id", SortOrder.ASC),
                YqlLimit.top(1))
                .stream()
                .findFirst();
    }

    public Optional<PostponeLaunch> firstPreviousLaunch(
            PostponeLaunch launch,
            List<VirtualType> virtualTypes
    ) {
        return find(
                YqlPredicate.where("id.processId").eq(launch.getId().getProcessId()),
                YqlPredicate.where("id.branch").eq(launch.getId().getBranch()),
                YqlPredicate.where("id.svnRevision").lt(launch.getId().getSvnRevision()),
                virtualTypes.isEmpty()
                        ? YqlPredicate.where("virtualType").isNull()
                        : YqlPredicateCi.in("virtualType", virtualTypes),
                YqlOrderBy.orderBy("id", SortOrder.DESC),
                YqlLimit.top(1))
                .stream()
                .findFirst();
    }

    public List<PostponeLaunch> findProcessingList(
            ProcessIdView processIdView,
            ArcBranch branch,
            long revisionFromExcluded,
            List<VirtualType> virtualTypes
    ) {
        return find(
                YqlPredicate.where("id.processId").eq(processIdView.getProcessId()),
                YqlPredicate.where("id.branch").eq(branch.getBranch()),
                YqlPredicate.where("id.svnRevision").gt(revisionFromExcluded),
                virtualTypes.isEmpty()
                        ? YqlPredicate.where("virtualType").isNull()
                        : YqlPredicateCi.in("virtualType", virtualTypes),
                YqlOrderBy.orderBy("id", SortOrder.ASC)
        );
    }

    public List<PostponeLaunch> findProcessingList(
            ProcessIdView processIdView,
            ArcBranch branch,
            List<VirtualType> virtualTypes,
            List<PostponeStatus> statuses
    ) {
        return find(
                YqlPredicate.where("id.processId").eq(processIdView.getProcessId()),
                YqlPredicate.where("id.branch").eq(branch.getBranch()),
                YqlPredicateCi.in("virtualType", virtualTypes),
                YqlPredicateCi.in("status", statuses),
                YqlOrderBy.orderBy("id", SortOrder.ASC)
        );
    }

    public void saveIfNotPresent(Launch launch) {
        var commit = launch.getVcsInfo().getCommit();
        Preconditions.checkState(commit != null,
                "Launch must be have commit: %s", commit);
        Preconditions.checkState(launch.getSelectedBranch() != null,
                "Launch must have selectedBranch: %s", launch.getId());
        Preconditions.checkState(commit.isTrunk(),
                "Launch must be started on trunk: %s", launch.getId());

        var id = PostponeLaunch.Id.of(
                launch.getId().getProcessId(),
                launch.getSelectedBranch(),
                commit.getSvnRevision()
        );

        var postponeLaunch = find(id);
        if (postponeLaunch.isPresent()) {
            log.warn("Unexpected attempt to store postpone launch which is already exists: {}", id);
            return;
        }

        var postpone = PostponeLaunch.builder()
                .id(id)
                .commitTime(commit.getCreateTime())
                .launchNumber(launch.getId().getLaunchNumber())
                .status(PostponeStatus.NEW)
                .virtualType(VirtualType.of(launch.getProcessId()))
                .build();
        save(postpone);
    }

    @Value
    public static class ProcessIdView implements Table.View {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;
    }
}
