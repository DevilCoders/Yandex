package ru.yandex.ci.core.db.table;

import java.nio.file.Path;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.apache.commons.lang3.StringUtils;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.ConfigStatePathView;
import ru.yandex.ci.core.db.model.ConfigStateProjectView;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class ConfigStateTable extends KikimrTableCi<ConfigState> {

    public ConfigStateTable(QueryExecutor executor) {
        super(ConfigState.class, executor);
    }

    public List<ConfigState> findByProject(String project, boolean includeInvalid) {
        return find(
                YqlPredicate.where("project").eq(project),
                YqlPredicateCi.notIn("status", getStatusesExcluded(includeInvalid)),
                YqlView.index(ConfigState.IDX_PROJECT_STATUS)
        );
    }

    public long countByProject(String project, boolean includeInvalid) {
        return count(
                YqlPredicate.where("project").eq(project),
                YqlPredicateCi.notIn("status", getStatusesExcluded(includeInvalid)),
                YqlView.index(ConfigState.IDX_PROJECT_STATUS)
        );
    }

    public List<ConfigState> findAll(boolean includeInvalid) {
        var where = filter(YqlPredicate.where("status").notIn(getStatusesExcluded(includeInvalid)));
        return find(where);
    }

    public List<ConfigState> findAllVisible(boolean includeInvalid) {
        var statusesExcluded = new HashSet<ConfigState.Status>();
        statusesExcluded.addAll(ConfigState.Status.hidden());
        statusesExcluded.addAll(getStatusesExcluded(includeInvalid));
        var where = filter(YqlPredicate.where("status").notIn(statusesExcluded));
        return find(where);
    }

    public List<String> findVisiblePaths() {
        return find(ConfigStatePathView.class, YqlPredicate.where("status").notIn(ConfigState.Status.hidden())).stream()
                .map(ConfigStatePathView::getConfigPath)
                .toList();
    }

    public List<String> findDraftPaths() {
        return find(ConfigStatePathView.class, YqlPredicate.where("status").eq(ConfigState.Status.DRAFT)).stream()
                .map(ConfigStatePathView::getConfigPath)
                .toList();
    }

    public List<String> listProjects(
            @Nullable ProjectFilter projectFilter,
            @Nullable String offsetProject,
            boolean includeInvalid,
            int limit
    ) {
        return listProjects(projectFilter, offsetProject, getStatusesExcluded(includeInvalid), limit);
    }

    public List<String> listProjects(
            @Nullable ProjectFilter projectFilter,
            @Nullable String offsetProject,
            Collection<ConfigState.Status> statusesExcluded,
            int limit
    ) {
        var parts = filter(limit);

        if (projectFilter != null) {
            parts.add(YqlPredicate.where("project").like(projectFilter.getFilter()).or(
                    YqlPredicate.where("project").in(projectFilter.getIncludeProjects())
            ));
        }

        if (StringUtils.isEmpty(offsetProject)) {
            parts.add(YqlPredicate.where("project").isNotNull());
        } else {
            parts.add(YqlPredicate.where("project").gt(offsetProject));
            parts.add(YqlView.index(ConfigState.IDX_PROJECT_STATUS));
        }
        if (!statusesExcluded.isEmpty()) {
            parts.add(YqlPredicateCi.notIn("status", statusesExcluded));
        }
        parts.add(YqlOrderBy.orderBy("project"));
        return findDistinct(ConfigStateProjectView.class, parts).stream()
                .map(ConfigStateProjectView::getProject)
                .collect(Collectors.toList());
    }

    public long countProjects(@Nullable ProjectFilter projectFilter, boolean includeInvalid) {
        var parts = filter();
        parts.add(YqlPredicate.where("project").isNotNull());
        if (projectFilter != null) {
            parts.add(YqlPredicate.where("project").like(projectFilter.getFilter()).or(
                    YqlPredicate.where("project").in(projectFilter.getIncludeProjects())
            ));
        }

        var statusesExcluded = getStatusesExcluded(includeInvalid);
        if (!statusesExcluded.isEmpty()) {
            parts.add(YqlPredicateCi.notIn("status", statusesExcluded));
        }
        parts.add(YqlView.index(ConfigState.IDX_PROJECT_STATUS));
        return countDistinct("project", parts);
    }

    public boolean upsertIfNewer(ConfigState newConfigState) {
        var oldConfigState = find(newConfigState.getId());
        if (oldConfigState.isEmpty() || newConfigState.getVersion() > oldConfigState.get().getVersion()) {
            if (oldConfigState.isPresent()) {
                save(newConfigState.withCreated(oldConfigState.get().getCreated()));
                return true;
            } else {
                save(newConfigState);
                return true;
            }
        }
        return false;
    }

    public Optional<ConfigState> find(Path configPath) {
        return find(ConfigState.Id.of(configPath));
    }

    public ConfigState get(Path configPath) {
        return get(ConfigState.Id.of(configPath));
    }

    /**
     * Состояния конфигов для списка путей. Если конфиг не найден для какого-то из путей,
     * он просто включается в результат.
     */
    public Map<Path, ConfigState> findByIds(Set<Path> configPaths) {
        Set<ConfigState.Id> ids = configPaths.stream()
                .map(ConfigState.Id::of)
                .collect(Collectors.toSet());

        return find(ids)
                .stream()
                .collect(Collectors.toMap(cs -> cs.getId().getConfigPath(), Function.identity()));
    }

    private static List<ConfigState.Status> getStatusesExcluded(boolean includeInvalid) {
        return includeInvalid
                ? List.of(ConfigState.Status.DELETED)
                : List.of(ConfigState.Status.DELETED, ConfigState.Status.INVALID);
    }
}
