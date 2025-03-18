package ru.yandex.ci.storage.core.db.model.common;

import java.time.Instant;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class StorageRevision {
    public static final StorageRevision EMPTY = new StorageRevision(Trunk.name(), "", 0, Instant.EPOCH);

    String branch;
    String revision;
    long revisionNumber;

    @Column(dbType = DbType.TIMESTAMP)
    Instant timestamp;

    public static StorageRevision from(String branch, ArcCommit leftCommit) {
        return new StorageRevision(
                branch,
                leftCommit.getCommitId(),
                leftCommit.getSvnRevision(),
                leftCommit.getCreateTime()
        );
    }

    public String getArcanumCommitId() {
        if (Trunk.name().equals(branch)) {
            return "r" + revisionNumber;
        } else {
            return revision;
        }
    }
}
