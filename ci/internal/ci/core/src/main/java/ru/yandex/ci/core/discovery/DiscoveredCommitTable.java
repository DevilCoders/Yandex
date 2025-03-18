package ru.yandex.ci.core.discovery;

import java.util.List;
import java.util.Optional;
import java.util.function.Function;

import com.google.common.base.Preconditions;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;

public class DiscoveredCommitTable extends KikimrTableCi<DiscoveredCommit> {
    public static final long LTE_COMMIT_ZERO = Long.MIN_VALUE;

    public DiscoveredCommitTable(QueryExecutor queryExecutor) {
        super(DiscoveredCommit.class, queryExecutor);
    }

    public Optional<DiscoveredCommit> findCommit(CiProcessId processId, OrderedArcRevision revision) {
        return find(DiscoveredCommit.Id.of(processId, revision));
    }

    public int count(CiProcessId processId, ArcBranch branch, long lteCommitNumber, long gtCommitNumber) {
        if (lteCommitNumber > 0 && gtCommitNumber > 0) {
            Preconditions.checkArgument(
                    lteCommitNumber >= gtCommitNumber,
                    "lteCommitNumber (%s) must be more or equal than gtCommitNumber (%s)",
                    lteCommitNumber, gtCommitNumber
            );
        }
        if (lteCommitNumber == LTE_COMMIT_ZERO) {
            lteCommitNumber = 0;
        } else if (lteCommitNumber <= 0) {
            lteCommitNumber = Long.MAX_VALUE;
        }

        return Math.toIntExact(count(
                YqlPredicate.where("id.processId").eq(processId.asString())
                        .and("id.branch").eq(branch.asString())
                        .and("id.commitNumber").lte(lteCommitNumber)
                        .and("id.commitNumber").gt(gtCommitNumber)
        ));
    }

    /**
     * Комиты возвращаются в порядке убывания (аналогично команде arc log). lteCommitNumber > gtCommitNumber
     *
     * @param processId       код процесса
     * @param branch          бранч
     * @param lteCommitNumber inclusive, -1 for any
     * @param gtCommitNumber  exclusive, -1 for any
     * @param limit           максимальное количество коммитов
     * @return список коммитов
     */
    public List<DiscoveredCommit> findCommits(
            CiProcessId processId,
            ArcBranch branch,
            long lteCommitNumber,
            long gtCommitNumber,
            int limit
    ) {
        return findCommits(processId, branch, lteCommitNumber, gtCommitNumber, limit, List.of());
    }

    public List<DiscoveredCommit> findCommits(
            CiProcessId processId,
            ArcBranch branch,
            long lteCommitNumber,
            long gtCommitNumber,
            int limit,
            List<YqlStatementPart<?>> additionalParts
    ) {
        if (lteCommitNumber > 0 && gtCommitNumber > 0) {
            Preconditions.checkArgument(
                    lteCommitNumber > gtCommitNumber,
                    "lteCommitNumber must be more than gtCommitNumber"

            );
        }

        boolean filter = lteCommitNumber > 0;
        if (lteCommitNumber == LTE_COMMIT_ZERO) {
            lteCommitNumber = 0;
            filter = true;
        }

        var parts = filter(limit);
        parts.add(YqlPredicate.where("id.processId").eq(processId.asString())
                .and("id.branch").eq(branch.asString()));
        if (filter) {
            parts.add(YqlPredicate.where("id.commitNumber").lte(lteCommitNumber));
        }
        parts.add(YqlPredicate.where("id.commitNumber").gt(gtCommitNumber));
        parts.add(YqlOrderBy.orderBy("id.commitNumber", YqlOrderBy.SortOrder.DESC));
        parts.addAll(additionalParts);
        return find(parts);
    }

    public List<DiscoveredCommit> findCommitsWithCancelledLaunches(
            CiProcessId processId,
            ArcBranch branch,
            long lteCommitNumber,
            long gtCommitNumber,
            int limit
    ) {
        List<YqlStatementPart<?>> parts = List.of(YqlPredicate.where("hasCancelledLaunches").eq(true));
        return findCommits(processId, branch, lteCommitNumber, gtCommitNumber, limit, parts);
    }

    public DiscoveredCommit updateOrCreate(
            CiProcessId processId,
            OrderedArcRevision revision,
            ConfigChange configChange,
            DiscoveryType discovery
    ) {
        return updateOrCreate(
                processId,
                revision,
                discoveredCommit -> {
                    var builder = discoveredCommit.map(DiscoveredCommitState::toBuilder)
                            .orElseGet(DiscoveredCommitState::builder)
                            .configChange(configChange);
                    switch (discovery) {
                        case DIR -> builder.dirDiscovery(true);
                        case GRAPH -> builder.graphDiscovery(true);
                        case PCI_DSS -> builder.pciDssDiscovery(true);
                        default -> throw new IllegalStateException("Unexpected discovery: " + discovery);
                    }
                    return builder.build();
                }
        );
    }

    public DiscoveredCommit updateOrCreate(
            CiProcessId processId,
            OrderedArcRevision revision,
            Function<Optional<DiscoveredCommitState>, DiscoveredCommitState> function
    ) {
        DiscoveredCommit.Id id = DiscoveredCommit.Id.of(processId, revision);
        Optional<DiscoveredCommit> current = find(id);
        DiscoveredCommitState newState = function.apply(current.map(DiscoveredCommit::getState));
        if (current.isPresent() && current.get().getState().equals(newState)) {
            return current.get();
        }
        int newStateVersion = current.map(c -> c.getStateVersion() + 1).orElse(1);
        DiscoveredCommit updated = DiscoveredCommit.of(processId, revision, newStateVersion, newState);
        // saveOrUpdate also updates projections
        return save(updated);
    }
}
