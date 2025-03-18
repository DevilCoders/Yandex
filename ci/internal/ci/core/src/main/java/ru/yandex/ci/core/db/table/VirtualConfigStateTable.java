package ru.yandex.ci.core.db.table;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.VirtualConfigState;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class VirtualConfigStateTable extends KikimrTableCi<VirtualConfigState> {

    public VirtualConfigStateTable(QueryExecutor executor) {
        super(VirtualConfigState.class, executor);
    }

    public List<VirtualConfigState> findByProject(String project) {
        return find(
                YqlPredicate.where("project").eq(project),
                YqlView.index(VirtualConfigState.IDX_PROJECT)
        );
    }

    public Map<VirtualCiProcessId.VirtualType, Long> countByProject(String project) {
        return this.groupBy(
                VirtualTypeView.class,
                List.of("virtualType"),
                List.of("virtualType"),
                List.of(
                        YqlPredicate.where("project").eq(project),
                        YqlView.index(VirtualConfigState.IDX_PROJECT)
                ))
                .stream()
                .collect(Collectors.toMap(VirtualTypeView::getVirtualType, VirtualTypeView::getCount));
    }

    public Optional<VirtualConfigState> find(Path configPath) {
        return find(VirtualConfigState.Id.of(configPath));
    }

    public VirtualConfigState get(Path configPath) {
        return get(VirtualConfigState.Id.of(configPath));
    }

    public List<VirtualConfigState> findByStatus(ConfigState.Status status) {
        return find(YqlPredicate.where("status").eq(status));
    }

    @Value
    public static class VirtualTypeView implements Table.View {

        @Column(dbType = DbType.UTF8)
        VirtualCiProcessId.VirtualType virtualType;

        @Column(name = "count", dbType = DbType.UINT64)
        long count;
    }
}
