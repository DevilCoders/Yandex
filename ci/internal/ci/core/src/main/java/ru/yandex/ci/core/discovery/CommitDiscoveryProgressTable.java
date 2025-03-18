package ru.yandex.ci.core.discovery;

import java.util.List;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class CommitDiscoveryProgressTable extends KikimrTableCi<CommitDiscoveryProgress> {

    public CommitDiscoveryProgressTable(QueryExecutor executor) {
        super(CommitDiscoveryProgress.class, executor);
    }

    public Optional<CommitDiscoveryProgress> find(String commitId) {
        return find(CommitDiscoveryProgress.Id.of(commitId));
    }

    public List<CommitDiscoveryProgress> findPciDssStateNotProcessed() {
        var predicate = YqlPredicate.where("pciDssState")
                .eq(CommitDiscoveryProgress.PciDssState.NOT_PROCESSED);
        return find(predicate);
    }

}
