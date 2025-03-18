package ru.yandex.ci.core.autocheck;

import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.ArcRevision;

public class AutocheckCommitTable extends KikimrTableCi<AutocheckCommit> {

    public AutocheckCommitTable(KikimrTable.QueryExecutor executor) {
        super(AutocheckCommit.class, executor);
    }

    public Optional<AutocheckCommit> find(ArcRevision revision) {
        return super.find(AutocheckCommit.Id.of(revision.getCommitId()));
    }

    public AutocheckCommit save(ArcRevision revision, AutocheckCommit.Status status) {
        return super.save(AutocheckCommit.of(
                AutocheckCommit.Id.of(revision.getCommitId()),
                status
        ));
    }
}
