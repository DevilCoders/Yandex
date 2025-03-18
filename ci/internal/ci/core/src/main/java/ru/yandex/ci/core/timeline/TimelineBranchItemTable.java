package ru.yandex.ci.core.timeline;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;
import com.google.common.collect.Sets;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.lang.NonNullApi;

import static java.util.stream.Collectors.toMap;

@Slf4j
@NonNullApi
public class TimelineBranchItemTable extends KikimrTableCi<TimelineBranchItem> {

    public TimelineBranchItemTable(QueryExecutor executor) {
        super(TimelineBranchItem.class, executor);
    }

    public List<TimelineBranchItem> findLastUpdatedInProcesses(List<CiProcessId> processIds, int groupLimit) {
        if (processIds.isEmpty()) {
            return List.of();
        }

        var processIdStrings = processIds.stream().map(CiProcessId::asString).toList();
        // split to batches to escape `ResultTruncatedException: Results was truncated to 1000 elements`
        var timelineBranchItemIds = Lists.partition(processIdStrings, YdbUtils.RESULT_ROW_LIMIT / groupLimit)
                .stream()
                .flatMap(processIdsBatch ->
                        byDateTable().topInGroups(
                                TimelineBranchItemByUpdateDate.class,
                                List.of(TimelineBranchItemByUpdateDate.PROCESS_ID_FIELD),
                                YqlOrderBy.orderBy(
                                        TimelineBranchItemByUpdateDate.UPDATE_DATE_FIELD,
                                        YqlOrderBy.SortOrder.DESC
                                ),
                                List.of(
                                        YqlPredicateCi.in(TimelineBranchItemByUpdateDate.PROCESS_ID_FIELD,
                                                processIdsBatch),
                                        YqlOrderBy.orderBy(TimelineBranchItemByUpdateDate.PROCESS_ID_FIELD)
                                ),
                                groupLimit
                        ).stream()
                )
                .map(TimelineBranchItemByUpdateDate::getItemId)
                .toList();

        return findByIdOrdered(timelineBranchItemIds);
    }

    public List<TimelineBranchItem> findByUpdateDate(CiProcessId processId,
                                                     @Nullable TimelineBranchItemByUpdateDate.Offset offset,
                                                     int limit) {
        var parts = filter(limit);
        parts.add(YqlPredicate.where(TimelineBranchItemByUpdateDate.PROCESS_ID_FIELD).eq(processId.asString()));
        if (offset != null) {
            parts.add(
                    YqlPredicate.where(TimelineBranchItemByUpdateDate.UPDATE_DATE_FIELD).lt(offset.getUpdateDate())
                            .or(
                                    YqlPredicate.where(TimelineBranchItemByUpdateDate.UPDATE_DATE_FIELD)
                                            .eq(offset.getUpdateDate())
                                            .and(YqlPredicate.where(TimelineBranchItemByUpdateDate.BRANCH_NAME_FIELD)
                                                    .gt(offset.getBranch().asString()))
                            )
            );
        }

        parts.add(YqlOrderBy.orderBy(
                new YqlOrderBy.SortKey(TimelineBranchItemByUpdateDate.UPDATE_DATE_FIELD, YqlOrderBy.SortOrder.DESC),
                new YqlOrderBy.SortKey(TimelineBranchItemByUpdateDate.BRANCH_NAME_FIELD, YqlOrderBy.SortOrder.ASC)
        ));

        List<TimelineBranchItem.Id> itemIds = byDateTable().find(parts)
                .stream()
                .map(TimelineBranchItemByUpdateDate::getItemId)
                .collect(Collectors.toList());

        return findByIdOrdered(itemIds);

    }


    @Override
    public void deleteAll() {
        super.deleteAll();
        byDateTable().deleteAll();
    }

    private List<TimelineBranchItem> findByIdOrdered(List<TimelineBranchItem.Id> ids) {
        var idsSet = new HashSet<>(ids);
        Map<TimelineBranchItem.Id, TimelineBranchItem> items = find(idsSet).stream()
                .collect(toMap(TimelineBranchItem::getId, Function.identity()));

        if (items.size() != idsSet.size()) {
            log.warn("Found less items than expected, {} not found", Sets.difference(idsSet, items.keySet()));
        }

        return ids.stream().map(items::get).toList();
    }

    public List<TimelineBranchItem> findByName(ArcBranch branch) {
        return find(
                YqlPredicate.where("id.branch").eq(branch.asString()),
                YqlView.index(TimelineBranchItem.IDX_BRANCH)
        );
    }

    public List<TimelineBranchItem> findByNames(CiProcessId processId, Collection<ArcBranch> names) {

        Set<TimelineBranchItem.Id> ids = StreamEx.of(names)
                .map(name -> TimelineBranchItem.Id.of(processId, name))
                .toSet();

        // тут необходим запрос веток из кеша ORM, однако он не используется при find(Set)
        return ids.stream()
                .map(this::findInternal)
                .filter(Objects::nonNull) // могут быть ветки, которые не представлены в текущем процессе
                .collect(Collectors.toList());
    }

    public List<TimelineBranchItem.Id> findIdsByNames(CiProcessId processId, Collection<ArcBranch> names) {
        Set<TimelineBranchItem.Id> ids = StreamEx.of(names)
                .map(name -> TimelineBranchItem.Id.of(processId, name))
                .toSet();
        return findIds(ids);
    }

    private KikimrTableCi<TimelineBranchItemByUpdateDate> byDateTable() {
        return new KikimrTableCi<>(TimelineBranchItemByUpdateDate.class, executor);
    }
}
