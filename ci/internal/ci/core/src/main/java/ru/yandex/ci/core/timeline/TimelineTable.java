package ru.yandex.ci.core.timeline;

import java.util.List;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlRawOrderBy;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;

public class TimelineTable extends KikimrTableCi<TimelineItemEntity> {
    public TimelineTable(QueryExecutor queryExecutor) {
        super(TimelineItemEntity.class, queryExecutor);
    }

    public static YqlPredicate startingAt(String processId, String branch, long revisionNumber, int itemNumber,
                                          Select select, boolean includeTrunk) {

        YqlPredicate idPredicate = switch (select) {
            case LATER -> YqlPredicate.gt("id.revision", revisionNumber)
                    .or(
                            YqlPredicate.eq("id.revision", revisionNumber)
                                    .and("id.itemNumber").gt(itemNumber)
                    );
            case EARLIER -> YqlPredicate.lt("id.revision", revisionNumber)
                    .or(
                            YqlPredicate.eq("id.revision", revisionNumber)
                                    .and("id.itemNumber").lt(itemNumber)
                    );
        };

        var branchField = YqlPredicate.where("id.branch");
        var showInBranchField = YqlPredicate.where("showInBranch");
        if (!includeTrunk) {
            idPredicate = idPredicate.and(branchField.eq(branch));
        } else {
            idPredicate = idPredicate.or(branchField.neq(branch));
        }
        return YqlPredicate.where("id.processId").eq(processId)
                .and(YqlPredicate.or(
                        showInBranchField.isNull().and(branchField.eq(branch)),
                        showInBranchField.eq(branch))
                )
                .and(idPredicate)
                .and("hidden").neq(true);
    }

    public static YqlRawOrderBy revisionAndNumberRaw(YqlOrderBy.SortOrder order) {
        return new YqlRawOrderBy("(branch = showInBranch) %s, revision %s, itemNumber %s"
                .formatted(order, order, order));
    }

    public List<TimelineItemEntity> getTimeline(CiProcessId processId, ArcBranch branch, Offset offset, int limit) {
        long revisionNumber = offset.getRevisionNumber();
        int itemNumber = offset.getItemNumber();

        var parts = filter(limit,
                startingAt(processId.asString(), branch.asString(), revisionNumber, itemNumber, Select.EARLIER,
                        true),
                revisionAndNumberRaw(YqlOrderBy.SortOrder.DESC));
        return find(parts);
    }

    public List<TimelineItemEntity> getAtRevision(CiProcessId processId, OrderedArcRevision revision) {
        YqlPredicate predicate = YqlPredicate.where("id.processId").eq(processId.asString())
                .and("id.branch").eq(revision.getBranch().asString())
                .and("id.revision").eq(revision.getNumber());

        return find(List.of(predicate, revisionAndNumberRaw(YqlOrderBy.SortOrder.DESC)));
    }

    public Optional<TimelineItemEntity> getAfter(TimelineItemEntity.Id id, boolean includeTrunk) {
        var revisionNumber = id.getRevision();
        var itemNumber = id.getItemNumber();

        return find(List.of(
                startingAt(id.getProcessId(), id.getBranch(), revisionNumber, itemNumber, Select.LATER, includeTrunk),
                revisionAndNumberRaw(YqlOrderBy.SortOrder.ASC),
                YqlLimit.top(1)
        )).stream().findFirst();
    }

    public Optional<TimelineItemEntity> getStableBefore(TimelineItemEntity.Id id) {
        var revisionNumber = id.getRevision();
        var itemNumber = id.getItemNumber();

        return find(List.of(
                startingAt(id.getProcessId(), id.getBranch(), revisionNumber, itemNumber, Select.EARLIER, true),
                YqlPredicate.where("inStable").eq(true),
                revisionAndNumberRaw(YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1)
        )).stream().findFirst();
    }

    public Optional<TimelineItemEntity> getFirstIn(CiProcessId processId, ArcBranch branch) {
        return getAfter(TimelineItemEntity.Id.of(processId, branch, 0, 0), false);
    }

    public Optional<TimelineItemEntity> getLastStarted(CiProcessId processId) {
        var ret = find(YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlPredicate.where("launch").isNotNull(),
                YqlOrderBy.orderBy("launchNumber", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1));
        return ret.stream().findFirst();
    }

    public enum Select {
        LATER,
        EARLIER,
    }

}
