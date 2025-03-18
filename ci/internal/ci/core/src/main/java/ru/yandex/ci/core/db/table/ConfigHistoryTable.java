package ru.yandex.ci.core.db.table;

import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import java.util.Set;

import yandex.cloud.repository.db.statement.Changeset;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigStatus;

public class ConfigHistoryTable extends KikimrTableCi<ConfigEntity> {

    public ConfigHistoryTable(QueryExecutor executor) {
        super(ConfigEntity.class, executor);
    }

    public static List<YqlStatementPart<?>> baseStatementParts(Path configPath, ArcBranch branch, int limit) {
        return filter(limit,
                YqlPredicate.where("id.configPath").eq(configPath.toString()),
                YqlPredicate.where("id.branch").eq(branch.asString()),
                YqlOrderBy.orderBy("id.commitNumber", YqlOrderBy.SortOrder.DESC));
    }

    public void updateSecurityState(Path configPath,
                                    OrderedArcRevision arcRevision,
                                    ConfigSecurityState securityState) {
        var id = ConfigEntity.Id.of(configPath, arcRevision);
        this.update(id, Changeset.setField("securityState", securityState)
                .set("status", ConfigStatus.READY));
    }

    public ConfigEntity getById(Path configPath, OrderedArcRevision revision) {
        return get(ConfigEntity.Id.of(configPath, revision));
    }

    public Optional<ConfigEntity> findById(Path configPath, OrderedArcRevision revision) {
        return find(ConfigEntity.Id.of(configPath, revision));
    }

    public Optional<ConfigEntity> findLastConfig(Path configPath, ArcBranch branch) {
        return findLastConfig(configPath, branch, Long.MAX_VALUE);
    }

    public Optional<ConfigEntity> findLastValidConfig(Path configPath, ArcBranch branch) {
        List<YqlStatementPart<?>> parts = baseStatementParts(configPath, branch, 1);
        parts.add(YqlPredicateCi.in("status", ConfigStatus.READY, ConfigStatus.SECURITY_PROBLEM));
        return find(parts).stream().findFirst();
    }

    public Optional<ConfigEntity> findLastReadyConfig(Path configPath, ArcBranch branch) {
        List<YqlStatementPart<?>> parts = baseStatementParts(configPath, branch, 1);
        parts.add(YqlPredicate.where("status").eq(ConfigStatus.READY));
        return find(parts).stream().findFirst();
    }

    public Optional<ConfigEntity> findLastConfig(
            Path configPath,
            OrderedArcRevision maxRevision,
            ConfigStatus... restrictStatuses
    ) {
        return findLastConfig(configPath, maxRevision.getBranch(), maxRevision.getNumber(), restrictStatuses);
    }

    public List<ConfigEntity> list(Path configPath, ArcBranch branch, long offsetCommitNumber, int limit) {
        List<YqlStatementPart<?>> parts = baseStatementParts(configPath, branch, limit);
        if (offsetCommitNumber > 0) {
            parts.add(YqlPredicate.where("id.commitNumber").lt(offsetCommitNumber));
        }
        return find(parts);
    }

    public Optional<ConfigEntity> findLastConfig(
            Path configPath,
            ArcBranch branch,
            long maxCommitNumber,
            ConfigStatus... restrictStatuses
    ) {
        List<YqlStatementPart<?>> parts = baseStatementParts(configPath, branch, 1);
        parts.add(YqlPredicate.where("id.commitNumber").lte(maxCommitNumber));
        if (restrictStatuses.length > 0) {
            parts.add(YqlPredicateCi.in("status", Set.of(restrictStatuses)));
        }
        return find(parts).stream().findFirst();
    }

    public long count(Path configPath, ArcBranch branch) {
        return count(
                YqlPredicate.where("id.configPath").eq(configPath.toString()),
                YqlPredicate.where("id.branch").eq(branch.asString())
        );
    }
}
