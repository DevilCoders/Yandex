package ru.yandex.ci.core.launch.versioning;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;

public class VersionsTable extends KikimrTableCi<Versions> {

    public VersionsTable(QueryExecutor queryExecutor) {
        super(Versions.class, queryExecutor);
    }

    public static Versions empty(CiProcessId processId, OrderedArcRevision revision) {
        return Versions.builder()
                .id(Versions.Id.of(processId, revision))
                .build();
    }

    public Versions getVersionsOrCreate(CiProcessId processId, OrderedArcRevision revision) {
        var id = Versions.Id.of(processId, revision);
        return find(id).orElseGet(() -> empty(processId, revision));
    }
}
