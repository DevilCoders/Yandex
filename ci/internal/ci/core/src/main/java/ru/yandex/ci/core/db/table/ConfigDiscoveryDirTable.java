package ru.yandex.ci.core.db.table;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.collect.Lists;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigDiscoveryDir;

public class ConfigDiscoveryDirTable extends KikimrTableCi<ConfigDiscoveryDir> {
    private static final int MAX_IN_QUERIES = YdbUtils.RESULT_ROW_LIMIT;

    public ConfigDiscoveryDirTable(QueryExecutor executor) {
        super(ConfigDiscoveryDir.class, executor);
    }

    public void deleteAllPath(CommitId commit, Path configPath) {
        var rows = find(
                YqlPredicate.where("id.configPath").eq(configPath.toString()),
                YqlPredicate.where("deleted").isNull());
        rows.stream()
                .map(row -> row.withDeleted(true))
                .forEach(this::save);
    }

    // Dirty hack
    public void updateAllPaths(CommitId commit,
                               Path configPath,
                               Collection<String> prefixes,
                               Consumer<Consumer<CiMainDb>> newTx) {
        if (prefixes.isEmpty()) {
            deleteAllPath(commit, configPath);
            return;
        }

        var config = configPath.toString();

        var currentPrefixes = find(YqlPredicate.where("id.configPath").eq(config),
                YqlPredicate.where("deleted").isNull()
        );
        var currentValues = currentPrefixes.stream()
                .collect(Collectors.toMap(ConfigDiscoveryDir::getId, Function.identity()));

        var newValues = prefixes.stream()
                .map(path -> ConfigDiscoveryDir.Id.of(config, path))
                .collect(Collectors.toSet());

        var toAdd = new HashSet<>(newValues);
        toAdd.removeAll(currentValues.keySet());

        var toRemove = new HashSet<>(currentValues.keySet());
        toRemove.removeAll(newValues);

        // YDB doing YDB thing
        // Multiple modification of table with secondary indexes is not supported yet

        // ORM doing ORM thing
        // Does not allow execute delete first or change the way we operate with table in any way

        var commitId = commit.getCommitId();

        if (!toAdd.isEmpty()) {
            if (toRemove.isEmpty()) {
                for (var id : toAdd) {
                    save(ConfigDiscoveryDir.of(id, commitId, null));
                }
            } else {
                newTx.accept(db -> {
                    db.tx(() -> {
                        var table = db.configDiscoveryDirs();
                        for (var id : toAdd) {
                            table.save(ConfigDiscoveryDir.of(id, commitId, null));
                        }
                    });
                });
            }
        }

        for (var id : toRemove) {
            save(ConfigDiscoveryDir.of(id, commitId, true));
        }
    }

    /**
     * @param affectedPaths list of dirs affected by commit
     * @return list of a.yaml files subscribed to provided dirs
     */
    public List<String> lookupAffectedConfigs(Set<String> affectedPaths) {
        // For a small number of path we could use StartsWith function, but if we have hundreds of directories...
        // Well, try to fetch them all?
        if (affectedPaths.isEmpty()) {
            return List.of();
        }

        Set<String> subDirs = new HashSet<>(affectedPaths.size() * 5);
        for (var path : affectedPaths) {
            fillSubDirs(path, subDirs);
        }

        List<ConfigPath> result = new ArrayList<>();
        for (var part : Lists.partition(List.copyOf(subDirs), MAX_IN_QUERIES)) {
            result.addAll(
                    findDistinct(ConfigPath.class, List.of(
                            YqlPredicateCi.in("id.pathPrefix", part),
                            YqlView.index(ConfigDiscoveryDir.IDX_PATH_PREFIX),
                            YqlPredicate.where("deleted").isNull(),
                            YqlOrderBy.orderBy("id.configPath"))
                    )
            );
        }
        return result.stream()
                .map(ConfigPath::getConfigPath)
                .collect(Collectors.toList());
    }

    public static void fillSubDirs(String path, Set<String> subDirs) {
        int pos = -1;
        while (true) {
            pos = path.indexOf('/', pos + 1);
            if (pos >= 0) {
                if (pos > 0) {
                    subDirs.add(path.substring(0, pos));
                }
            } else {
                break;
            }
        }
        subDirs.add(path);
    }

    @Value
    static class ConfigPath implements View {
        @Column(name = "id_configPath", dbType = DbType.STRING)
        String configPath;
    }
}
